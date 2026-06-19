#pragma once
#include "job_scheduler/models/Job.hpp"
#include "job_scheduler/repository/IJobRepository.hpp"
#include <queue>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

namespace job_scheduler {


class Scheduler {
public:
    using DueCallback = std::function<void(Job)>;

    explicit Scheduler(IJobReader& reader, IJobWriter& writer);
    ~Scheduler();

    /** Rebuild heap from all active/paused jobs in the database. */
    void load_from_repository();

    /** Add a newly created job to the heap. */
    void schedule(const Job& job);

    /** Remove a job from future execution (does not touch DB). */
    void cancel(const std::string& job_id);

    /** Pause – job stays in heap but will be skipped when due. */
    void pause(const std::string& job_id);

    /** Resume – job is re-inserted with its stored next_run_time. */
    void resume(const Job& job);

    /** Register the callback fired when a job is due. */
    void set_due_callback(DueCallback cb);

    /** Start the background timing loop. */
    void start();

    /** Stop the background timing loop (blocks until thread exits). */
    void stop();

private:
    struct HeapEntry {
        std::chrono::system_clock::time_point next_run;
        std::string job_id;

        // Min-heap: smallest time_point first
        bool operator>(const HeapEntry& o) const { return next_run > o.next_run; }
    };

    using MinHeap = std::priority_queue<HeapEntry,
                                        std::vector<HeapEntry>,
                                        std::greater<HeapEntry>>;

    void loop();

    IJobReader& reader_;
    IJobWriter& writer_;

    MinHeap  heap_;
    std::mutex mutex_;
    std::condition_variable cv_;

    std::thread thread_;
    std::atomic<bool> running_{false};

    DueCallback on_due_;
};

} // namespace job_scheduler
