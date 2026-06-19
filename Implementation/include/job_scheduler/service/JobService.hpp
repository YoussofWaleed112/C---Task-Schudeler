#pragma once
#include "job_scheduler/models/Job.hpp"
#include "job_scheduler/repository/IJobRepository.hpp"
#include "job_scheduler/scheduler/Scheduler.hpp"
#include "job_scheduler/executor/Executor.hpp"
#include <string>
#include <vector>
#include <optional>

namespace job_scheduler {

struct CreateJobRequest {
    std::string name;
    std::string schedule_type;   // "one_time" | "interval" | "cron"
    std::string schedule_expr;   // timestamp | seconds | cron expr
    std::string execution_target;
    int  max_retries{0};
    int  retry_delay_seconds{3};
};

struct ServiceResult {
    bool        ok{true};
    std::string error;
    std::optional<Job> job;
};


class JobService {
public:
    JobService(IJobReader& reader,
               IJobWriter& writer,
               Scheduler&  scheduler,
               Executor&   executor);

    ServiceResult create_job(const CreateJobRequest& req);
    std::vector<Job> list_jobs();
    ServiceResult get_job(const std::string& id);
    ServiceResult pause_job(const std::string& id);
    ServiceResult resume_job(const std::string& id);
    ServiceResult delete_job(const std::string& id);

private:
    static std::string generate_id();

    IJobReader& reader_;
    IJobWriter& writer_;
    Scheduler&  scheduler_;
    Executor&   executor_;
};

} // namespace job_scheduler
