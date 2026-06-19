#include "job_scheduler/api/HttpServer.hpp"
#include "job_scheduler/api/JobController.hpp"
#include "httplib.h"
#include <iostream>

namespace job_scheduler {

HttpServer::HttpServer(const std::string& host,
                       int port,
                       JobController& controller)
    : host_(host), port_(port), controller_(controller),
      server_(std::make_unique<httplib::Server>()) {}

HttpServer::~HttpServer() { stop(); }

void HttpServer::start() {
    controller_.register_routes(*server_);

    thread_ = std::thread([this] {
        std::cout << "[http] Listening on " << host_ << ":" << port_ << "\n";
        if (!server_->listen(host_.c_str(), port_)) {
            std::cerr << "[http] Failed to listen on port " << port_ << "\n";
        }
    });
}

void HttpServer::stop() {
    server_->stop();
    if (thread_.joinable()) thread_.join();
}

} // namespace job_scheduler
