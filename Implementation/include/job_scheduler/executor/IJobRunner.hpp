#pragma once
#include "job_scheduler/models/job.hpp"
#include <string>

namespace job_scheduler {

class IJobRunner {
public:
    virtual ~IJobRunner() = default;
    virtual bool run(const Job& job) = 0;
};

} // namespace job_scheduler
