#include "catch.hpp"
#include "job_scheduler/models/OneTimeSchedule.hpp"
#include "job_scheduler/models/IntervalSchedule.hpp"
#include "job_scheduler/models/CronSchedule.hpp"
#include <chrono>
#include <cmath>

TEST_CASE("OneTimeSchedule - Calculates Next Run Time", "[schedule]") {
    auto now      = std::chrono::system_clock::now();
    auto taskTime = now + std::chrono::hours(2);

    OneTimeSchedule schedule(taskTime);

    SECTION("Current time is BEFORE the task time") {
        auto nextRun = schedule.nextRunAfter(now);
        REQUIRE(nextRun.has_value());
        REQUIRE(nextRun.value() == taskTime);
    }

    SECTION("Current time is AFTER the task time") {
        auto currentTime = taskTime + std::chrono::hours(1);
        auto nextRun = schedule.nextRunAfter(currentTime);
        REQUIRE(!nextRun.has_value());
    }
}

TEST_CASE("IntervalSchedule - Calculates Next Run Time", "[schedule]") {
    int interval = 30;
    IntervalSchedule schedule(interval);

    auto currentTime = std::chrono::system_clock::now();
    auto nextRun = schedule.nextRunAfter(currentTime);

    REQUIRE(nextRun.has_value());

    auto expectedTime = currentTime + std::chrono::seconds(interval);
    REQUIRE(nextRun.value() == expectedTime);
}

TEST_CASE("CronSchedule - Calculates Next Run Time", "[schedule]") {
    SECTION("Wildcard expression runs every minute") {
        CronSchedule cron("* * * * *");
        auto now = std::chrono::system_clock::now();
        auto nextRun = cron.nextRunAfter(now);

        REQUIRE(nextRun.has_value());

        auto expectedTime = now + std::chrono::minutes(1);
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
                        nextRun.value() - expectedTime).count();
        REQUIRE(std::abs(diff) <= 60);
    }

    SECTION("Validation of cron expressions") {
        REQUIRE(CronSchedule::isValid("30 12 * *")    == false); // only 4 fields
        REQUIRE(CronSchedule::isValid("30 12 * * *")  == true);  // 5 fields
    }
}
