#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace job_scheduler {


class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    void submit(std::function<void()> task);

    size_t pending() const;

private:
    void worker_loop();

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> queue_;
    mutable std::mutex                mutex_;
    std::condition_variable           cv_;
    std::atomic<bool>                 stop_{false};
};

} // namespace job_scheduler
