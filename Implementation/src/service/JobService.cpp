#include "job_scheduler/service/JobService.hpp"
#include "job_scheduler/scheduler/ScheduleParser.hpp"
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <stdexcept>

namespace job_scheduler {

JobService::JobService(IJobReader& reader,
                       IJobWriter& writer,
                       Scheduler&  scheduler,
                       Executor&   executor)
    : reader_(reader), writer_(writer),
      scheduler_(scheduler), executor_(executor) {}

// ── Helpers ────────────────────────────────────────────────────────────────

std::string JobService::generate_id() {
    // Simple UUID-like hex string using random_device + timestamp
    static std::mt19937_64 rng(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    uint64_t a = rng();
    uint64_t b = rng();
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << a
        << std::setw(16) << b;
    return oss.str();
}

// ── CRUD operations ────────────────────────────────────────────────────────

ServiceResult JobService::create_job(const CreateJobRequest& req) {
    ServiceResult result;

    // Validate and build the schedule
    std::shared_ptr<ISchedule> schedule;
    try {
        schedule = ScheduleParser::parse(req.schedule_type, req.schedule_expr);
    } catch (const std::exception& e) {
        result.ok    = false;
        result.error = e.what();
        return result;
    }

    if (req.name.empty()) {
        result.ok    = false;
        result.error = "Job name must not be empty";
        return result;
    }
    if (req.execution_target.empty()) {
        result.ok    = false;
        result.error = "execution_target must not be empty";
        return result;
    }

    // Calculate initial next_run_time
    auto now  = std::chrono::system_clock::now();
    auto next = schedule->next_run(now);
    if (!next.has_value()) {
        result.ok    = false;
        result.error = "Schedule produces no valid run time";
        return result;
    }

    Job job;
    job.id               = generate_id();
    job.name             = req.name;
    job.status           = JobStatus::Active;
    job.schedule_type    = req.schedule_type;
    job.schedule_expr    = req.schedule_expr;
    job.execution_target = req.execution_target;
    job.retry_policy.max_retries = req.max_retries;
    job.retry_policy.retry_delay = std::chrono::seconds(req.retry_delay_seconds);
    job.retry_count      = 0;
    job.next_run_time    = *next;
    job.schedule         = schedule;

    writer_.save(job);
    scheduler_.schedule(job);

    result.job = job;
    return result;
}

std::vector<Job> JobService::list_jobs() {
    return reader_.find_all();
}

ServiceResult JobService::get_job(const std::string& id) {
    ServiceResult result;
    auto job = reader_.find_by_id(id);
    if (!job) {
        result.ok    = false;
        result.error = "Job not found: " + id;
        return result;
    }
    result.job = *job;
    return result;
}

ServiceResult JobService::pause_job(const std::string& id) {
    ServiceResult result;
    auto job = reader_.find_by_id(id);
    if (!job) {
        result.ok    = false;
        result.error = "Job not found: " + id;
        return result;
    }
    if (job->status == JobStatus::Paused) {
        result.ok    = false;
        result.error = "Job is already paused";
        return result;
    }
    if (job->status == JobStatus::Completed ||
        job->status == JobStatus::Failed    ||
        job->status == JobStatus::Cancelled) {
        result.ok    = false;
        result.error = "Cannot pause a job in status: " + to_string(job->status);
        return result;
    }

    writer_.update_status(id, JobStatus::Paused);
    scheduler_.pause(id);

    job->status = JobStatus::Paused;
    result.job = *job;
    return result;
}

ServiceResult JobService::resume_job(const std::string& id) {
    ServiceResult result;
    auto job = reader_.find_by_id(id);
    if (!job) {
        result.ok    = false;
        result.error = "Job not found: " + id;
        return result;
    }
    if (job->status != JobStatus::Paused) {
        result.ok    = false;
        result.error = "Job is not paused to resume ,the current status is: " + to_string(job->status);
        return result;
    }

    writer_.update_status(id, JobStatus::Active);
    job->status = JobStatus::Active;
    scheduler_.resume(*job);

    result.job = *job;
    return result;
}

ServiceResult JobService::delete_job(const std::string& id) {
    ServiceResult result;
    auto job = reader_.find_by_id(id);
    if (!job) {
        result.ok    = false;
        result.error = "Job not found: " + id;
        return result;
    }

    scheduler_.cancel(id);
    writer_.remove(id);
    return result;
}

} // namespace job_scheduler
