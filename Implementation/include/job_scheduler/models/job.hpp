#pragma once
#include "JobStatus.hpp"
#include "RetryPolicy.hpp"
#include "ISchedule.hpp"
#include <string>
#include <memory>

struct Job {
    std::string              id;
    std::string              name;
    JobStatus                status;
    std::shared_ptr<ISchedule> schedule;
    std::string              schedule_type;   // "one_time" | "interval" | "cron"
    std::string              schedule_expr;   // serialized schedule data
    RetryPolicy              retryPolicy;
    std::string              executionTarget;
    TimePoint                nextRunTime;
    int                      retryCount = 0;
};
