#include "job_scheduler/db/Connection.hpp"
#include "job_scheduler/repository/SqliteJobRepository.hpp"
#include "job_scheduler/executor/ThreadPool.hpp"
#include "job_scheduler/executor/ShellJobRunner.hpp"
#include "job_scheduler/executor/Executor.hpp"
#include "job_scheduler/scheduler/Scheduler.hpp"
#include "job_scheduler/service/JobService.hpp"
#include "job_scheduler/api/JobController.hpp"
#include "job_scheduler/api/HttpServer.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

using namespace job_scheduler;

static HttpServer* g_server = nullptr;

void signal_handler(int) {
    std::cout << "\n[main] Shutting down...\n";
    if (g_server) g_server->stop();
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // --- Database ---
    auto conn = std::make_shared<db::Connection>("jobs.db");
    SqliteJobRepository repo(conn);

    // --- Scheduler ---
    Scheduler scheduler(repo, repo);

    // --- Executor ---
    ThreadPool    pool(4);
    ShellJobRunner runner;
    Executor executor(pool, runner, repo, repo, scheduler);

    // Wire the due-job callback: scheduler → executor
    scheduler.set_due_callback([&executor](Job job) {
        executor.dispatch(std::move(job));
    });

    // --- Reload persisted jobs ---
    scheduler.load_from_repository();
    scheduler.start();

    // --- HTTP API ---
    JobService    service(repo, repo, scheduler, executor);
    JobController controller(service);
    HttpServer    server("0.0.0.0", 8080, controller);

    g_server = &server;
    server.start();

    std::cout << "[main] Job scheduler running. POST /jobs to create a job.\n";

    // Block forever (or until signal)
    std::this_thread::sleep_for(std::chrono::hours(24 * 365));

    scheduler.stop();
    return 0;
}
