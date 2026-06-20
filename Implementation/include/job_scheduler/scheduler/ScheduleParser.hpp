#pragma once
#include "job_scheduler/models/ISchedule.hpp"
#include "job_scheduler/models/OneTimeSchedule.hpp"
#include "job_scheduler/models/IntervalSchedule.hpp"
#include "job_scheduler/models/CronSchedule.hpp"
#include <string>
#include <memory>
#include <stdexcept>

namespace job_scheduler {

class ScheduleParser {
public:
    static std::shared_ptr<ISchedule> parse(const std::string& type,
                                            const std::string& expr)
    {
        if (type == "one_time") {
            return parse_one_time(expr);
        } else if (type == "interval") {
            return parse_interval(expr);
        } else if (type == "cron") {
            return parse_cron(expr);
        }
        throw std::invalid_argument(
            "Unknown schedule type '" + type +
            "'. Valid types: one_time, interval, cron");
    }

private:
    static std::shared_ptr<ISchedule> parse_one_time(const std::string& expr) {
        int64_t ts = 0;
        try {
            ts = std::stoll(expr);
        } catch (...) {
            throw std::invalid_argument(
                "one_time schedule_expr must be a Unix timestamp (integer). Got: " + expr);
        }
        if (ts <= 0) {
            throw std::invalid_argument(
                "one_time schedule_expr must be a positive Unix timestamp. Got: " + expr);
        }
        auto tp = std::chrono::system_clock::from_time_t(static_cast<time_t>(ts));
        return std::make_shared<OneTimeSchedule>(tp);
    }

    static std::shared_ptr<ISchedule> parse_interval(const std::string& expr) {
        long long seconds = 0;
        try {
            seconds = std::stoll(expr);
        } catch (...) {
            throw std::invalid_argument(
                "interval schedule_expr must be an integer (seconds). Got: " + expr);
        }
        if (seconds <= 0) {
            throw std::invalid_argument(
                "interval schedule_expr must be > 0 seconds. Got: " + expr);
        }
        // IntervalSchedule takes int seconds
        return std::make_shared<IntervalSchedule>(static_cast<int>(seconds));
    }

    static std::shared_ptr<ISchedule> parse_cron(const std::string& expr) {
        return std::make_shared<CronSchedule>(expr);
    }
};

} // namespace job_scheduler
