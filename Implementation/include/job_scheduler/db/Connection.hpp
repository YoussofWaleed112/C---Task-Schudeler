#pragma once
#include <sqlite3.h>
#include <string>
#include <stdexcept>

namespace db {

class Connection {
public:
    explicit Connection(const std::string& path) {
        if (sqlite3_open(path.c_str(), &_db) != SQLITE_OK) {
            std::string err = sqlite3_errmsg(_db);
            sqlite3_close(_db);
            throw std::runtime_error("Cannot open database: " + err);
        }
        sqlite3_exec(_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    }

    ~Connection() {
        if (_db) sqlite3_close(_db);
    }


    Connection(const Connection&)            = delete;
    Connection& operator=(const Connection&) = delete;

    sqlite3* handle() const { return _db; }

private:
    sqlite3* _db = nullptr;
};

} 
