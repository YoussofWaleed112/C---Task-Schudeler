#pragma once
#include "ThreadPool.hpp"
#include "IJobRunner.hpp"
#include "job_scheduler/repository/IJobReader.hpp"
#include "job_scheduler/repository/IJobWriter.hpp"
#include "job_scheduler/scheduler/ScheduleParser.hpp"
#include <functional>
#include <memory>

namespace job_scheduler {

// Forward declare to avoid circular include
class Scheduler;

class Executor {
public:
    Executor(ThreadPool& pool,
             IJobRunner& runner,
             IJobWriter& writer,
             IJobReader& reader,
             Scheduler&  scheduler);

    /** Called by Scheduler when a job is due. */
    void dispatch(Job job);

private:
    void execute(Job job);

    ThreadPool& pool_;
    IJobRunner& runner_;
    IJobWriter& writer_;
    IJobReader& reader_;
    Scheduler&  scheduler_;
};

} // namespace job_scheduler
