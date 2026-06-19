#include "catch.hpp"
#include "job_scheduler/repository/SqliteJobRepository.hpp"
#include "job_scheduler/db/Connection.hpp"
#include "job_scheduler/models/IntervalSchedule.hpp"
#include <memory>
#include <chrono>

TEST_CASE("SqliteJobRepository - CRUD Operations & Database Persistence", "[repository]") {

    // 1. Setup a temporary in-memory SQLite database connection for isolation
    auto conn = std::make_shared<db::Connection>(":memory:");
    SqliteJobRepository repo(conn); // Schema is automatically initialized here

    // 2. Prepare mock data for a sample Job
    Job mockJob;
    mockJob.id = "job-123";
    mockJob.name = "Database Backup Task";
    mockJob.status = JobStatus::Active;
    mockJob.executionTarget = "scripts/backup.sh";
    mockJob.nextRunTime = std::chrono::system_clock::now();
    mockJob.retryCount = 0;
    mockJob.retryPolicy.maxRetries = 3;
    mockJob.retryPolicy.backoffSeconds = 5;
    mockJob.schedule = std::make_shared<IntervalSchedule>(60); // Repeat every 60 seconds

    // ============================================================================
    // SECTION 1: Create and Read Operations (CRUD - C & R)
    // ============================================================================
    SECTION("Inserting and retrieving a job by ID") {
        // Create the job in the repository
        repo.create(mockJob);

        // Read the job back from the database
        auto retrievedOpt = repo.getById("job-123");

        REQUIRE(retrievedOpt.has_value());

        Job retrievedJob = retrievedOpt.value();
        REQUIRE(retrievedJob.id == mockJob.id);
        REQUIRE(retrievedJob.name == mockJob.name);
        REQUIRE(retrievedJob.status == JobStatus::Active);
        REQUIRE(retrievedJob.schedule->type() == "interval");
    }

    // ============================================================================
    // SECTION 2: Update Operation (CRUD - U)
    // ============================================================================
    SECTION("Updating an existing job's details and status") {
        repo.create(mockJob);

        // Modify job details
        mockJob.name = "Updated Backup Task";
        mockJob.status = JobStatus::Running;
        repo.update(mockJob);

        // Verify changes are persisted correctly
        auto retrievedOpt = repo.getById("job-123");
        REQUIRE(retrievedOpt.has_value());
        REQUIRE(retrievedOpt.value().name == "Updated Backup Task");
        REQUIRE(retrievedOpt.value().status == JobStatus::Running);
    }

    // ============================================================================
    // SECTION 3: Delete Operation (CRUD - D)
    // ============================================================================
    SECTION("Removing a job from the database") {
        repo.create(mockJob);

        // Verify it exists first
        REQUIRE(repo.getById("job-123").has_value());

        // Remove the job and verify it's gone
        repo.remove("job-123");
        auto retrievedOpt = repo.getById("job-123");
        REQUIRE(!retrievedOpt.has_value());
    }

    // ============================================================================
    // SECTION 4: Querying Lists (listAll & listDue)
    // ============================================================================
    SECTION("Listing all jobs and detection of due jobs") {
        repo.create(mockJob);

        // Check listAll contains the inserted job
        auto allJobs = repo.listAll();
        REQUIRE(allJobs.size() == 1);

        // Check listDue catches jobs scheduled to run in the past or exactly now
        auto targetTime = mockJob.nextRunTime + std::chrono::seconds(10);
        auto dueJobs = repo.listDue(targetTime);
        REQUIRE(dueJobs.size() == 1);
    }
}