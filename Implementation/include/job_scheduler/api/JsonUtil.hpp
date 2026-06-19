#pragma once

#include <string>
#include <chrono>
#include <sstream>
#include <map>
#include <stdexcept>
#include <algorithm>

namespace job_scheduler {
namespace json_util {

// ── Serialization ──────────────────────────────────────────────────────────

inline std::string escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else                out += c;
    }
    return out;
}

inline std::string kv(const std::string& key, const std::string& val) {
    return "\"" + key + "\":\"" + escape(val) + "\"";
}
inline std::string kv_int(const std::string& key, int val) {
    return "\"" + key + "\":" + std::to_string(val);
}
inline std::string kv_int64(const std::string& key, int64_t val) {
    return "\"" + key + "\":" + std::to_string(val);
}
inline std::string kv_bool(const std::string& key, bool val) {
    return "\"" + key + "\":" + (val ? "true" : "false");
}

inline std::string obj(std::initializer_list<std::string> fields) {
    std::string out = "{";
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        if (it != fields.begin()) out += ",";
        out += *it;
    }
    return out + "}";
}

inline std::string error_obj(const std::string& msg) {
    return obj({kv("error", msg)});
}

// ── Simple parsing ─────────────────────────────────────────────────────────

/**
 * Very small key-value extractor.
 * Handles flat JSON objects with string and integer values only,
 * which is all we need for request bodies.
 */
class SimpleParser {
public:
    explicit SimpleParser(const std::string& body) : body_(body) {}

    std::string get_str(const std::string& key, const std::string& def = "") const {
        std::string search = "\"" + key + "\"";
        auto pos = body_.find(search);
        if (pos == std::string::npos) return def;
        auto colon = body_.find(':', pos + search.size());
        if (colon == std::string::npos) return def;
        auto quote1 = body_.find('"', colon + 1);
        if (quote1 == std::string::npos) return def;
        auto quote2 = quote1 + 1;
        while (quote2 < body_.size()) {
            if (body_[quote2] == '\\') { quote2 += 2; continue; }
            if (body_[quote2] == '"')  break;
            ++quote2;
        }
        if (quote2 >= body_.size()) return def;
        // Unescape
        std::string raw = body_.substr(quote1 + 1, quote2 - quote1 - 1);
        std::string out;
        for (size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] == '\\' && i+1 < raw.size()) {
                char c = raw[++i];
                if      (c == '"')  out += '"';
                else if (c == '\\') out += '\\';
                else if (c == 'n')  out += '\n';
                else if (c == 'r')  out += '\r';
                else if (c == 't')  out += '\t';
                else                { out += '\\'; out += c; }
            } else {
                out += raw[i];
            }
        }
        return out;
    }

    int get_int(const std::string& key, int def = 0) const {
        std::string search = "\"" + key + "\"";
        auto pos = body_.find(search);
        if (pos == std::string::npos) return def;
        auto colon = body_.find(':', pos + search.size());
        if (colon == std::string::npos) return def;
        // Skip whitespace
        size_t vpos = colon + 1;
        while (vpos < body_.size() && std::isspace(body_[vpos])) ++vpos;
        if (vpos >= body_.size()) return def;
        // Check it's not a string (starts with quote)
        if (body_[vpos] == '"') return def;
        try { return std::stoi(body_.substr(vpos)); }
        catch (...) { return def; }
    }

private:
    std::string body_;
};

} // namespace json_util
} // namespace job_scheduler
