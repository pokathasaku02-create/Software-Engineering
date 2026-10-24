// Source of "now". Injected so tests can fix the time and the deadline rules stay testable.
#pragma once
#include "Types.h"

namespace lms {

class IClock {
public:
    virtual ~IClock() = default;
    virtual DateTime now() const = 0;
};

// Current school wall-clock time (see Types.h for the meaning of DateTime).
class SystemClock : public IClock {
public:
    DateTime now() const override;
};

// Fixed, adjustable clock for tests.
class FixedClock : public IClock {
public:
    explicit FixedClock(DateTime t) : t_(t) {}
    DateTime now() const override { return t_; }
    void set(DateTime t) { t_ = t; }
    void advanceDays(int days) { t_ += days * kSecondsPerDay; }
private:
    DateTime t_;
};

}  // namespace lms
