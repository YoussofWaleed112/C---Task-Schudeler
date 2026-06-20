#include "job_scheduler/service/JobService.hpp"
#include "job_scheduler/scheduler/ScheduleParser.hpp"
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <stdexcept>

namespace job_scheduler {

// Helper: status enum to string
static std::string status_to_string(JobStatus s) {
    switch (s) {
        case JobStatus::Active:    return "active";
        case JobStatus::Paused:    return "paused";
        case JobStatus::Running:   return "running";
        case JobStatus::Completed: return "completed";
        case JobStatus::Failed:    return "failed";
        case JobStatus::Cancelled: return "cancelled";
        default:                   return "active";
    }
}

JobService::JobService(IJobReader& reader,
                       IJobWriter& writer,
                       Scheduler&  scheduler,
                       Executor&   executor)
    : reader_(reader), writer_(writer),
      scheduler_(scheduler), executor_(executor) {}

std::string JobService::generate_id() {
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

ServiceResult JobService::create_job(const CreateJobRequest& req) {
    ServiceResult result;

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

    std::shared_ptr<ISchedule> schedule;
    try {
        schedule = ScheduleParser::parse(req.schedule_type, req.schedule_expr);
    } catch (const std::exception& e) {
        result.ok    = false;
        result.error = e.what();
        return result;
    }

    auto now  = std::chrono::system_clock::now();
    auto next = schedule->nextRunAfter(now);
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
    job.executionTarget  = req.execution_target;
    job.retryPolicy.maxRetries     = req.max_retries;
    job.retryPolicy.backoffSeconds = req.retry_delay_seconds;
    job.retryCount       = 0;
    job.nextRunTime      = *next;
    job.schedule         = schedule;

    writer_.create(job);
    scheduler_.schedule(job);

    result.job = job;
    return result;
}

std::vector<Job> JobService::list_jobs() {
    return reader_.listAll();
}

ServiceResult JobService::get_job(const std::string& id) {
    ServiceResult result;
    auto job = reader_.getById(id);
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
    auto job = reader_.getById(id);
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
        result.error = "Cannot pause a job in status: " + status_to_string(job->status);
        return result;
    }

    job->status = JobStatus::Paused;
    writer_.update(*job);
    scheduler_.pause(id);

    result.job = *job;
    return result;
}

ServiceResult JobService::resume_job(const std::string& id) {
    ServiceResult result;
    auto job = reader_.getById(id);
    if (!job) {
        result.ok    = false;
        result.error = "Job not found: " + id;
        return result;
    }
    if (job->status != JobStatus::Paused) {
        result.ok    = false;
        result.error = "Job is not paused; current status: " + status_to_string(job->status);
        return result;
    }

    job->status = JobStatus::Active;
    writer_.update(*job);
    scheduler_.resume(*job);

    result.job = *job;
    return result;
}

ServiceResult JobService::delete_job(const std::string& id) {
    ServiceResult result;
    auto job = reader_.getById(id);
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
