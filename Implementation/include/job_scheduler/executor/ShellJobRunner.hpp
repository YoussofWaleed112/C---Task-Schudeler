#pragma once
#include "IJobRunner.hpp"
#include <cstdlib>
#include <iostream>

namespace job_scheduler {

/**
 * ShellJobRunner – runs execution_target as a shell command via system().
    This is an example of a job runner
 */
class ShellJobRunner : public IJobRunner {
public:
    bool run(const Job& job) override {
        std::cout << "[runner] Executing job '" << job.name
                  << "' → " << job.execution_target << "\n";
        int rc = std::system(job.execution_target.c_str());
        return rc == 0;
    }
};

} // namespace job_scheduler
