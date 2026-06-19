#include "job_scheduler/executor/ThreadPool.hpp"

namespace job_scheduler {

ThreadPool::ThreadPool(size_t num_threads) {
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push(std::move(task));
    }
    cv_.notify_one();
}

size_t ThreadPool::pending() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return queue_.size();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cv_.wait(lk, [this]{ return stop_ || !queue_.empty(); });
            if (stop_ && queue_.empty()) return;
            task = std::move(queue_.front());
            queue_.pop();
        }
        task();
    }
}

} // namespace job_scheduler
