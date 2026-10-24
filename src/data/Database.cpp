#include "Database.h"

#include <sqlite3.h>

#include <utility>

#include "../domain/Exceptions.h"
#include "Schema.h"

namespace lms {
namespace {

[[noreturn]] void fail(sqlite3* db, const std::string& what) {
    throw DatabaseException(what + ": " + (db ? sqlite3_errmsg(db) : "no database"));
}

}  // namespace

// ---------------- Statement ----------------

Statement::Statement(sqlite3* db, const std::string& sql) : db_(db) {
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) fail(db_, "Could not prepare statement");
}

Statement::~Statement() {
    if (stmt_) sqlite3_finalize(stmt_);
}

Statement::Statement(Statement&& other) noexcept : db_(other.db_), stmt_(other.stmt_) { other.stmt_ = nullptr; }

Statement& Statement::bind(int index, long long value) {
    if (sqlite3_bind_int64(stmt_, index, value) != SQLITE_OK) fail(db_, "Could not bind integer");
    return *this;
}

Statement& Statement::bind(int index, const std::string& value) {
    if (sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK)
        fail(db_, "Could not bind text");
    return *this;
}

Statement& Statement::bindNull(int index) {
    if (sqlite3_bind_null(stmt_, index) != SQLITE_OK) fail(db_, "Could not bind null");
    return *this;
}

Statement& Statement::bind(int index, const std::optional<int>& value) {
    return value ? bind(index, *value) : bindNull(index);
}

bool Statement::step() {
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    fail(db_, "Database error");
}

void Statement::run() { step(); }

long long Statement::columnInt(int col) const { return sqlite3_column_int64(stmt_, col); }

std::string Statement::columnText(int col) const {
    const unsigned char* t = sqlite3_column_text(stmt_, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

bool Statement::isNull(int col) const { return sqlite3_column_type(stmt_, col) == SQLITE_NULL; }

std::optional<int> Statement::columnOptInt(int col) const {
    if (isNull(col)) return std::nullopt;
    return static_cast<int>(columnInt(col));
}

// ---------------- Database ----------------

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        const std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        if (db_) sqlite3_close(db_);
        db_ = nullptr;
        throw DatabaseException("Could not open database '" + path + "': " + msg);
    }
    exec("PRAGMA foreign_keys = ON;");
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::initialiseSchema() { exec(kSchemaSql); }

Statement Database::prepare(const std::string& sql) { return Statement(db_, sql); }

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        const std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw DatabaseException("Database error: " + msg);
    }
}

long long Database::lastInsertId() const { return sqlite3_last_insert_rowid(db_); }

void Database::begin() { exec("BEGIN IMMEDIATE;"); }
void Database::commit() { exec("COMMIT;"); }
void Database::rollback() { exec("ROLLBACK;"); }

}  // namespace lms
