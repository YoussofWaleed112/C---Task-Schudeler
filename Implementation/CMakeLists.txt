cmake_minimum_required(VERSION 3.16)
project(job_scheduler LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Vendored SQLite ---
add_library(sqlite3 STATIC third_party/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party/sqlite3)

find_package(Threads REQUIRED)
target_link_libraries(sqlite3 PUBLIC Threads::Threads)

# --- Project headers ---
include_directories(include)
include_directories(tests) # Included your tests folder header path

# --- Source files ---
set(SOURCES
    src/main.cpp
    src/models/OneTimeSchedule.cpp
    src/models/IntervalSchedule.cpp
    src/models/CronSchedule.cpp
    src/repository/SqliteJobRepository.cpp
)

add_executable(job_scheduler ${SOURCES})
target_link_libraries(job_scheduler PRIVATE sqlite3)

# ============================================================================
# --- Testing Framework Config ---
# ============================================================================
enable_testing()

# Defining the sources required to build the test executable safely
set(TEST_SOURCES
    tests/test_executor.cpp
    tests/test_repository.cpp
    tests/test_schedule.cpp
    tests/test_scheduler.cpp
    src/models/OneTimeSchedule.cpp
    src/models/IntervalSchedule.cpp
    src/models/CronSchedule.cpp
    src/repository/SqliteJobRepository.cpp
)

# Creating the independent test executable linked with sqlite3
add_executable(run_tests ${TEST_SOURCES})
target_link_libraries(run_tests PRIVATE sqlite3)

# Registering the tests with CMake test engine
add_test(NAME AllTests COMMAND run_tests)