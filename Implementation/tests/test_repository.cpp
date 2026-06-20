#include "catch.hpp"
#include "job_scheduler/repository/SqliteJobRepository.hpp"
#include "job_scheduler/db/Connection.hpp"
#include "job_scheduler/models/IntervalSchedule.hpp"
#include <memory>
#include <chrono>

TEST_CASE("SqliteJobRepository - CRUD Operations", "[repository]") {

    auto conn = std::make_shared<db::Connection>(":memory:");
    SqliteJobRepository repo(conn);

    Job mockJob;
    mockJob.id              = "job-123";
    mockJob.name            = "Database Backup Task";
    mockJob.status          = JobStatus::Active;
    mockJob.executionTarget = "scripts/backup.sh";
    mockJob.nextRunTime     = std::chrono::system_clock::now();
    mockJob.retryCount      = 0;
    mockJob.retryPolicy.maxRetries     = 3;
    mockJob.retryPolicy.backoffSeconds = 5;
    mockJob.schedule        = std::make_shared<IntervalSchedule>(60);

    SECTION("Inserting and retrieving a job by ID") {
        repo.create(mockJob);

        auto retrieved = repo.getById("job-123");
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->id   == mockJob.id);
        REQUIRE(retrieved->name == mockJob.name);
        REQUIRE(retrieved->status == JobStatus::Active);
        REQUIRE(retrieved->schedule->type() == "interval");
    }

    SECTION("Updating an existing job") {
        repo.create(mockJob);

        mockJob.name   = "Updated Backup Task";
        mockJob.status = JobStatus::Running;
        repo.update(mockJob);

        auto retrieved = repo.getById("job-123");
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->name   == "Updated Backup Task");
        REQUIRE(retrieved->status == JobStatus::Running);
    }

    SECTION("Removing a job") {
        repo.create(mockJob);
        REQUIRE(repo.getById("job-123").has_value());

        repo.remove("job-123");
        REQUIRE(!repo.getById("job-123").has_value());
    }

    SECTION("Listing all jobs and due jobs") {
        repo.create(mockJob);

        auto allJobs = repo.listAll();
        REQUIRE(allJobs.size() == 1);

        auto targetTime = mockJob.nextRunTime + std::chrono::seconds(10);
        auto dueJobs = repo.listDue(targetTime);
        REQUIRE(dueJobs.size() == 1);
    }
}
