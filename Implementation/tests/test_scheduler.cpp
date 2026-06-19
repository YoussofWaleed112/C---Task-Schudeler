#include "catch.hpp"
#include "job_scheduler/models/OneTimeSchedule.hpp"
#include "job_scheduler/models/IntervalSchedule.hpp"
#include "job_scheduler/models/CronSchedule.hpp"
#include <chrono>

TEST_CASE("OneTimeSchedule - Calculates Next Run Time", "[schedule]") {

   
    auto now = std::chrono::system_clock::now();
    auto taskTime = now + std::chrono::hours(2);

   
    OneTimeSchedule schedule(taskTime);

    SECTION("Current time is BEFORE the task time") {
        auto currentTime = now;
        auto nextRun = schedule.nextRunAfter(currentTime);

       
        REQUIRE(nextRun.has_value());
        REQUIRE(nextRun.value() == taskTime);
    }

    SECTION("Current time is AFTER the task time") {
        auto currentTime = taskTime + std::chrono::hours(1);
        auto nextRun = schedule.nextRunAfter(currentTime);

      
        REQUIRE(!nextRun.has_value());
    }
}

//IntervalSchedule Test Case
TEST_CASE("IntervalSchedule - Calculates Next Run Time Based on Interval", "[schedule]") {

	// assuming we have an IntervalSchedule class that takes an interval in seconds and calculates the next run time based on that interval
    int interval = 30;
    IntervalSchedule schedule(interval);

    auto currentTime = std::chrono::system_clock::now();

    auto nextRun = schedule.nextRunAfter(currentTime);

	// assert that the next run time is correctly calculated based on the interval
    REQUIRE(nextRun.has_value());

	// assuming the next run time should be current time + interval
    auto expectedTime = currentTime + std::chrono::seconds(interval);
    REQUIRE(nextRun.value() == expectedTime);
}

// 3. CronSchedule Tests
// ============================================================================
TEST_CASE("CronSchedule - Calculates Next Run Time Based on Cron Expression", "[schedule]") {

    SECTION("Wildcard Expression (* * * * *) runs every minute") {
        CronSchedule cron("* * * * *");

        auto now = std::chrono::system_clock::now();
        auto nextRun = cron.nextRunAfter(now);

        REQUIRE(nextRun.has_value());

        // Cron scanner starts checking from (from + 1 minute)
        auto expectedTime = now + std::chrono::minutes(1);

        // Allow a small delta of 2 seconds due to system clock precision
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(nextRun.value() - expectedTime).count();
        REQUIRE(std::abs(diff) <= 2);
    }

    SECTION("Validation of cron expressions") {
        // Validation checks for format correctness
        REQUIRE(CronSchedule::isValid("30 12 * *") == false);  // Invalid: missing field
        REQUIRE(CronSchedule::isValid("30 12 * * *") == true); // Valid: exactly 5 fields
    }
}