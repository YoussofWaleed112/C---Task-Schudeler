#include "job_scheduler/api/JobController.hpp"
#include "job_scheduler/api/JsonUtil.hpp"
#include "httplib.h"
#include <sstream>

namespace job_scheduler {

using namespace json_util;

JobController::JobController(JobService& service) : service_(service) {}

// ── JSON helpers ────────────────────────────────────────────────────────────

std::string JobController::job_to_json(const Job& job) {
    int64_t nrt = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            job.next_run_time.time_since_epoch()).count());

    return obj({
        kv      ("id",               job.id),
        kv      ("name",             job.name),
        kv      ("status",           to_string(job.status)),
        kv      ("schedule_type",    job.schedule_type),
        kv      ("schedule_expr",    job.schedule_expr),
        kv      ("execution_target", job.execution_target),
        kv_int  ("max_retries",      job.retry_policy.max_retries),
        kv_int  ("retry_delay_s",    static_cast<int>(
                                         job.retry_policy.retry_delay.count())),
        kv_int  ("retry_count",      job.retry_count),
        kv_int64("next_run_time",    nrt)
    });
}

std::string JobController::jobs_to_json(const std::vector<Job>& jobs) {
    std::string out = "[";
    for (size_t i = 0; i < jobs.size(); ++i) {
        if (i) out += ",";
        out += job_to_json(jobs[i]);
    }
    return out + "]";
}

// ── Route registration ──────────────────────────────────────────────────────

void JobController::register_routes(httplib::Server& svr) {
    // POST /jobs – create a new job
    svr.Post("/jobs", [this](const httplib::Request& req,
                              httplib::Response& res) {
        SimpleParser p(req.body);

        CreateJobRequest cr;
        cr.name             = p.get_str("name");
        cr.schedule_type    = p.get_str("schedule_type");
        cr.schedule_expr    = p.get_str("schedule_expr");
        cr.execution_target = p.get_str("execution_target");
        cr.max_retries      = p.get_int("max_retries",       0);
        cr.retry_delay_seconds = p.get_int("retry_delay_seconds", 5);

        auto result = service_.create_job(cr);
        if (!result.ok) {
            res.status = 400;
            res.set_content(error_obj(result.error), "application/json");
            return;
        }
        res.status = 201;
        res.set_content(job_to_json(*result.job), "application/json");
    });

    // GET /jobs – list all jobs
    svr.Get("/jobs", [this](const httplib::Request&,
                             httplib::Response& res) {
        auto jobs = service_.list_jobs();
        res.status = 200;
        res.set_content(jobs_to_json(jobs), "application/json");
    });

    // GET /jobs/:id – get one job
    svr.Get(R"(/jobs/([a-fA-F0-9]+))",
            [this](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        auto result = service_.get_job(id);
        if (!result.ok) {
            res.status = 404;
            res.set_content(error_obj(result.error), "application/json");
            return;
        }
        res.status = 200;
        res.set_content(job_to_json(*result.job), "application/json");
    });

    // POST /jobs/:id/pause
    svr.Post(R"(/jobs/([a-fA-F0-9]+)/pause)",
             [this](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        auto result = service_.pause_job(id);
        if (!result.ok) {
            res.status = (result.error.find("not found") != std::string::npos) ? 404 : 400;
            res.set_content(error_obj(result.error), "application/json");
            return;
        }
        res.status = 200;
        res.set_content(job_to_json(*result.job), "application/json");
    });

    // POST /jobs/:id/resume
    svr.Post(R"(/jobs/([a-fA-F0-9]+)/resume)",
             [this](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        auto result = service_.resume_job(id);
        if (!result.ok) {
            res.status = (result.error.find("not found") != std::string::npos) ? 404 : 400;
            res.set_content(error_obj(result.error), "application/json");
            return;
        }
        res.status = 200;
        res.set_content(job_to_json(*result.job), "application/json");
    });

    // DELETE /jobs/:id
    svr.Delete(R"(/jobs/([a-fA-F0-9]+))",
               [this](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        auto result = service_.delete_job(id);
        if (!result.ok) {
            res.status = 404;
            res.set_content(error_obj(result.error), "application/json");
            return;
        }
        res.status = 204; // No Content
    });
}

} // namespace job_scheduler
