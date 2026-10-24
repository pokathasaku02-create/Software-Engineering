// Lets services make several repository calls atomic without knowing about SQLite.
#pragma once

namespace lms {

class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;
    virtual void begin() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

// RAII helper: rolls back automatically unless commit() was called.
class ScopedTransaction {
public:
    explicit ScopedTransaction(ITransactionManager& tm) : tm_(tm) { tm_.begin(); }
    ~ScopedTransaction() {
        if (!done_) {
            try { tm_.rollback(); } catch (...) {}
        }
    }
    ScopedTransaction(const ScopedTransaction&) = delete;
    ScopedTransaction& operator=(const ScopedTransaction&) = delete;
    void commit() { tm_.commit(); done_ = true; }
private:
    ITransactionManager& tm_;
    bool done_ = false;
};

}  // namespace lms
