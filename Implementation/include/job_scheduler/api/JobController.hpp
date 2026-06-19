#pragma once
#include "job_scheduler/service/JobService.hpp"

namespace httplib {
class Server;
}

namespace job_scheduler {


class JobController {
public:
    explicit JobController(JobService& service);

    /** Register all routes on the provided httplib::Server. */
    void register_routes(httplib::Server& server);

private:
    JobService& service_;

    static std::string job_to_json(const Job& job);
    static std::string jobs_to_json(const std::vector<Job>& jobs);
};

} // namespace job_scheduler
