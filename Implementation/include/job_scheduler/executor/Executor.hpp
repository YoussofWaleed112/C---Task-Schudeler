#pragma once
#include "ThreadPool.hpp"
#include "IJobRunner.hpp"
#include "job_scheduler/repository/IJobRepository.hpp"
#include "job_scheduler/scheduler/ScheduleParser.hpp"
#include <functional>
#include <memory>

namespace job_scheduler {

class Executor {
public:

    Executor(ThreadPool& pool,
             IJobRunner& runner,
             IJobWriter& writer,
             IJobReader& reader,
             Scheduler& scheduler);

    void dispatch(Job job);


private:
    void execute(Job job);

    ThreadPool& pool_;
    IJobRunner& runner_;
    IJobWriter& writer_;
    IJobReader& reader_;
    Scheduler& scheduler_;
};

} // namespace job_scheduler
