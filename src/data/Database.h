// Thin RAII wrapper around the SQLite C API. All SQL is run through prepared statements
// with bound parameters; no query is ever built by string concatenation of user input.
#pragma once
#include <optional>
#include <string>

#include "ITransactionManager.h"

struct sqlite3;
struct sqlite3_stmt;

namespace lms {

class Statement {
public:
    Statement(sqlite3* db, const std::string& sql);
    ~Statement();
    Statement(Statement&& other) noexcept;
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement& operator=(Statement&&) = delete;

    // Parameter indexes start at 1.
    Statement& bind(int index, long long value);
    Statement& bind(int index, int value) { return bind(index, static_cast<long long>(value)); }
    Statement& bind(int index, const std::string& value);
    Statement& bindNull(int index);
    Statement& bind(int index, const std::optional<int>& value);

    bool step();   // true while a row is available
    void run();    // for statements that return no rows

    // Column indexes start at 0.
    long long columnInt(int col) const;
    std::string columnText(int col) const;
    bool isNull(int col) const;
    std::optional<int> columnOptInt(int col) const;

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

class Database : public ITransactionManager {
public:
    explicit Database(const std::string& path);   // ":memory:" for tests
    ~Database() override;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void initialiseSchema();
    Statement prepare(const std::string& sql);
    void exec(const std::string& sql);
    long long lastInsertId() const;

    void begin() override;
    void commit() override;
    void rollback() override;

private:
    sqlite3* db_ = nullptr;
};

}  // namespace lms
