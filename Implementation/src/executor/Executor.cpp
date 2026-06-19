#include "job_scheduler/executor/Executor.hpp"
#include "job_scheduler/models/OneTimeSchedule.hpp"
#include "job_scheduler/models/IntervalSchedule.hpp"
#include "job_scheduler/models/CronSchedule.hpp"
#include <iostream>
#include <thread>

namespace job_scheduler {

Executor::Executor(ThreadPool& pool,
                   IJobRunner& runner,
                   IJobWriter& writer,
                   IJobReader& reader,
                   Scheduler& scheduler)
    : pool_(pool), runner_(runner), writer_(writer), reader_(reader), scheduler_(scheduler) {}


void Executor::dispatch(Job job) {
    // Mark running in DB immediately (before handing to pool)
    writer_.update_status(job.id, JobStatus::Running);
    job.status = JobStatus::Running;

    pool_.submit([this, j = std::move(job)]() mutable {
        execute(std::move(j));
    });
}

void Executor::execute(Job job) {
    // Re-read from DB in case status changed while waiting in queue
    auto job = reader_.find_by_id(job.id);
    if (!job || job->status == JobStatus::Cancelled ||
                  job->status == JobStatus::Paused) {
        // Restore active status so it can be resumed later
        if (job && job->status == JobStatus::Paused) {
            writer_.update_status(job.id, JobStatus::Paused);
        }
        return;
    }
    job = *job;

    // Build the schedule object from persisted type + expr
    std::shared_ptr<ISchedule> schedule;
    try {
        schedule = ScheduleParser::parse(job.schedule_type, job.schedule_expr);
    } catch (const std::exception& e) {
        std::cerr << "[from executor] Failed to rebuild schedule for job "
                  << job.id << ": " << e.what() << "\n";
        writer_.update_status(job.id, JobStatus::Failed);
        return;
    }

    bool success = runner_.run(job);

    if (success) {
        // Calculate the next run time
        auto now = std::chrono::system_clock::now();
        auto next = schedule->next_run(now);

        if (!next.has_value() || job.schedule_type == "one_time") {
            // One-time job or schedule exhausted → mark completed
            writer_.update_status(job.id, JobStatus::Completed);
            std::cout << "[from executor] Job '" << job.name << "' completed.\n";
        } else {
            // Recurring job → reschedule
            writer_.update_status(job.id, JobStatus::Active);
            writer_.update_next_run(job.id, *next);
            writer_.update_retry_count(job.id, 0); // reset retries on success

            job.status = JobStatus::Active;
            job.next_run_time = *next;
            job.retry_count = 0;
            job.schedule = schedule;

            std::cout << "Job '" << job.name << "' succeeded, rescheduled.\n";
            scheduler_.reschedule(job);
        }
    } else {
        // Failure path
        int new_count = job.retry_count + 1;

        if (new_count <= job.retry_policy.max_retries) {
            // Schedule a retry
            writer_.update_retry_count(job.id, new_count);
            auto retry_at = std::chrono::system_clock::now() +
                            job.retry_policy.retry_delay;
            writer_.update_next_run(job.id, retry_at);
            writer_.update_status(job.id, JobStatus::Active);

            job.retry_count = new_count;
            job.next_run_time = retry_at;
            job.status = JobStatus::Active;
            job.schedule = schedule;

            std::cout << "[from executor] Job '" << job.name
                      << "' failed (attempt " << new_count
                      << "/" << job.retry_policy.max_retries
                      << "), will retry.\n";
            scheduler_.reschedule(job);
        } else {
            writer_.update_status(job.id, JobStatus::Failed);
            std::cout << "[from executor] Job '" << job.name
                      << "' failed – retries exhausted.\n";
        }
    }
}

} // namespace job_scheduler
