#include "job_scheduler/scheduler/Scheduler.hpp"
#include <iostream>

namespace job_scheduler {

Scheduler::Scheduler(IJobReader& reader, IJobWriter& writer, Executor& executor)
    : reader_(reader), writer_(writer), executor_(executor) {}

Scheduler::~Scheduler() { stop(); }

void Scheduler::load_from_repository() {
    std::lock_guard<std::mutex> lk(mutex_);
    // Clear the heap and rebuild
    heap_ = MinHeap{};
    for (const auto& job : reader_.find_all()) {
        if (job.status == JobStatus::Active ||
            job.status == JobStatus::Running) {
            heap_.push({job.next_run_time, job.id});
        }
        // Paused jobs are NOT added; resume() will re-add them
    }
    cv_.notify_one();
}

void Scheduler::schedule(const Job& job) {
    std::lock_guard<std::mutex> lk(mutex_);
    heap_.push({job.next_run_time, job.id});
    cv_.notify_one();
}

void Scheduler::cancel(const std::string& /*job_id*/) {
    // The heap has no efficient erase; cancelled jobs are filtered in loop()
    // by checking their DB status before firing the callback.
    cv_.notify_one(); // wake the loop so it re-evaluates the top
}

void Scheduler::pause(const std::string& /*job_id*/) {
    // Same lazy-deletion strategy: the loop checks DB status before firing.
    cv_.notify_one();
}

void Scheduler::resume(const Job& job) {
    std::lock_guard<std::mutex> lk(mutex_);
    heap_.push({job.next_run_time, job.id});
    cv_.notify_one();
}

void Scheduler::start() {
    running_ = true;
    thread_ = std::thread(&Scheduler::loop, this);
}

void Scheduler::stop() {
    running_ = false;
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

void Scheduler::loop() {
    while (running_) {
        std::unique_lock<std::mutex> lk(mutex_);

        if (heap_.empty()) {
            // Sleep indefinitely until something is scheduled or we shut down
            cv_.wait(lk, [this]{ return !running_ || !heap_.empty(); });
            continue;
        }

        auto now = std::chrono::system_clock::now();
        auto top_time = heap_.top().next_run;

        if (top_time > now) {
            // Sleep until the next job is due (or woken early)
            cv_.wait_until(lk, top_time);
            continue;
        }

        // Pop everything that is due right now
        std::vector<std::string> due_ids;
        while (!heap_.empty() && heap_.top().next_run <= now) {
            due_ids.push_back(heap_.top().job_id);
            heap_.pop();
        }
        lk.unlock();

        // For each due entry, check the DB for current status
        for (const auto& id : due_ids) {
            auto maybe_job = reader_.find_by_id(id);
            if (!maybe_job) continue; // deleted

            const Job& job = *maybe_job;
            if (job.status == JobStatus::Paused   ||
                job.status == JobStatus::Cancelled ||
                job.status == JobStatus::Completed ||
                job.status == JobStatus::Failed) {
                continue; 
            }

            executor_.execute(job);
        }
    }
}

} // namespace job_scheduler
