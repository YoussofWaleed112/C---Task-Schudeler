#pragma once
#include <string>
#include <thread>
#include <memory>

namespace httplib { class Server; }

namespace job_scheduler {

class JobController;


class HttpServer {
public:
    HttpServer(const std::string& host, int port, JobController& controller);
    ~HttpServer();

    void start();   
    void stop();

private:
    std::string     host_;
    int             port_;
    JobController&  controller_;

    std::unique_ptr<httplib::Server> server_;
    std::thread     thread_;
};

} // namespace job_scheduler
            
