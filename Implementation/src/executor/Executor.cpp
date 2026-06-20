#include "job_scheduler/executor/Executor.hpp"
#include "job_scheduler/scheduler/Scheduler.hpp"
#include <iostream>

namespace job_scheduler {

Executor::Executor(ThreadPool& pool,
                   IJobRunner& runner,
                   IJobWriter& writer,
                   IJobReader& reader,
                   Scheduler&  scheduler)
    : pool_(pool), runner_(runner), writer_(writer),
      reader_(reader), scheduler_(scheduler) {}

void Executor::dispatch(Job job) {
    job.status = JobStatus::Running;
    // Update DB status before handing to pool
    Job updated = job;
    updated.status = JobStatus::Running;
    writer_.update(updated);

    pool_.submit([this, j = std::move(job)]() mutable {
        execute(std::move(j));
    });
}

void Executor::execute(Job job) {
    // Re-read from DB to get latest status (may have been paused/cancelled)
    auto maybe_job = reader_.getById(job.id);
    if (!maybe_job) return; // deleted

    if (maybe_job->status == JobStatus::Cancelled ||
        maybe_job->status == JobStatus::Paused) {
        // If it was paused while queued, reset to paused so it won't re-run
        Job tmp = *maybe_job;
        writer_.update(tmp);
        return;
    }

    // Rebuild schedule from persisted type + expr
    std::shared_ptr<ISchedule> schedule;
    try {
        schedule = ScheduleParser::parse(job.schedule_type, job.schedule_expr);
    } catch (const std::exception& e) {
        std::cerr << "[executor] Failed to rebuild schedule for job "
                  << job.id << ": " << e.what() << "\n";
        job.status = JobStatus::Failed;
        writer_.update(job);
        return;
    }

    bool success = runner_.run(job);

    if (success) {
        auto now  = std::chrono::system_clock::now();
        auto next = schedule->nextRunAfter(now);

        if (!next.has_value() || job.schedule_type == "one_time") {
            job.status = JobStatus::Completed;
            writer_.update(job);
            std::cout << "[executor] Job '" << job.name << "' completed.\n";
        } else {
            job.status       = JobStatus::Active;
            job.nextRunTime  = *next;
            job.retryCount   = 0;
            job.schedule     = schedule;
            writer_.update(job);
            std::cout << "[executor] Job '" << job.name << "' succeeded, rescheduled.\n";
            scheduler_.reschedule(job);
        }
    } else {
        int new_count = job.retryCount + 1;

        if (new_count <= job.retryPolicy.maxRetries) {
            auto retry_at = std::chrono::system_clock::now() +
                            std::chrono::seconds(job.retryPolicy.backoffSeconds);
            job.retryCount  = new_count;
            job.nextRunTime = retry_at;
            job.status      = JobStatus::Active;
            job.schedule    = schedule;
            writer_.update(job);

            std::cout << "[executor] Job '" << job.name
                      << "' failed (attempt " << new_count
                      << "/" << job.retryPolicy.maxRetries
                      << "), will retry.\n";
            scheduler_.reschedule(job);
        } else {
            job.status = JobStatus::Failed;
            writer_.update(job);
            std::cout << "[executor] Job '" << job.name
                      << "' failed – retries exhausted.\n";
        }
    }
}

} // namespace job_scheduler
