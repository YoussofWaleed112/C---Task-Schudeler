#include "job_scheduler/scheduler/Scheduler.hpp"
#include <iostream>

namespace job_scheduler {

Scheduler::Scheduler(IJobReader& reader, IJobWriter& writer)
    : reader_(reader), writer_(writer) {}

Scheduler::~Scheduler() { stop(); }

void Scheduler::load_from_repository() {
    std::lock_guard<std::mutex> lk(mutex_);
    heap_ = MinHeap{};
    for (const auto& job : reader_.listAll()) {
        if (job.status == JobStatus::Active ||
            job.status == JobStatus::Running) {
            heap_.push({job.nextRunTime, job.id});
        }
    }
    cv_.notify_one();
}

void Scheduler::schedule(const Job& job) {
    std::lock_guard<std::mutex> lk(mutex_);
    heap_.push({job.nextRunTime, job.id});
    cv_.notify_one();
}

void Scheduler::cancel(const std::string& /*job_id*/) {
    // Lazy deletion: the loop checks DB status before firing the callback.
    cv_.notify_one();
}

void Scheduler::pause(const std::string& /*job_id*/) {
    cv_.notify_one();
}

void Scheduler::resume(const Job& job) {
    std::lock_guard<std::mutex> lk(mutex_);
    heap_.push({job.nextRunTime, job.id});
    cv_.notify_one();
}

void Scheduler::reschedule(const Job& job) {
    std::lock_guard<std::mutex> lk(mutex_);
    heap_.push({job.nextRunTime, job.id});
    cv_.notify_one();
}

void Scheduler::set_due_callback(DueCallback cb) {
    on_due_ = std::move(cb);
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
            cv_.wait(lk, [this]{ return !running_ || !heap_.empty(); });
            continue;
        }

        auto now      = std::chrono::system_clock::now();
        auto top_time = heap_.top().next_run;

        if (top_time > now) {
            cv_.wait_until(lk, top_time);
            continue;
        }

        std::vector<std::string> due_ids;
        while (!heap_.empty() && heap_.top().next_run <= now) {
            due_ids.push_back(heap_.top().job_id);
            heap_.pop();
        }
        lk.unlock();

        for (const auto& id : due_ids) {
            auto maybe_job = reader_.getById(id);
            if (!maybe_job) continue;

            const Job& job = *maybe_job;
            if (job.status == JobStatus::Paused   ||
                job.status == JobStatus::Cancelled ||
                job.status == JobStatus::Completed ||
                job.status == JobStatus::Failed) {
                continue;
            }

            if (on_due_) {
                on_due_(job);
            }
        }
    }
}

} // namespace job_scheduler
