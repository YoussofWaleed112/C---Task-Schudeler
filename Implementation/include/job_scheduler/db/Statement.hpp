#pragma once
#include "Connection.hpp"
#include <string>
#include <stdexcept>

namespace db {

class Statement {
public:
    Statement(const Connection& conn, const std::string& sql) {
        if (sqlite3_prepare_v2(conn.handle(), sql.c_str(), -1, &_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement: " +
                                     std::string(sqlite3_errmsg(conn.handle())));
        }
    }

    ~Statement() {
        if (_stmt) sqlite3_finalize(_stmt);
    }

    Statement(const Statement&)            = delete;
    Statement& operator=(const Statement&) = delete;


    void bindText(int col, const std::string& val) {
        sqlite3_bind_text(_stmt, col, val.c_str(), -1, SQLITE_TRANSIENT);
    }
    void bindInt(int col, int val)         { sqlite3_bind_int(_stmt, col, val); }
    void bindInt64(int col, int64_t val)   { sqlite3_bind_int64(_stmt, col, val); }

   
    bool step() { return sqlite3_step(_stmt) == SQLITE_ROW; }

    void exec() {
        if (sqlite3_step(_stmt) != SQLITE_DONE)
            throw std::runtime_error("Statement execution failed");
    }


    std::string columnText(int col)  { return reinterpret_cast<const char*>(sqlite3_column_text(_stmt, col)); }
    int         columnInt(int col)   { return sqlite3_column_int(_stmt, col); }
    int64_t     columnInt64(int col) { return sqlite3_column_int64(_stmt, col); }

    void reset() { sqlite3_reset(_stmt); }

private:
    sqlite3_stmt* _stmt = nullptr;
};

}  
