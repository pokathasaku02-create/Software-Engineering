// Strategy pattern: how a late submission's mark is reduced.
#pragma once
#include <string>

namespace lms {

class LatePenaltyPolicy {
public:
    virtual ~LatePenaltyPolicy() = default;
    // daysLate <= 0 means the work was on time.
    virtual int applyPenalty(int rawMark, int maxMark, int daysLate) const = 0;
    virtual std::string describe() const = 0;
};

// Head of Department's rule: lose 10% of the available mark for each day late, up to
// three days; after that the work receives zero.
class TenPercentPerDayPolicy : public LatePenaltyPolicy {
public:
    TenPercentPerDayPolicy(double ratePerDay = 0.10, int maxDaysLate = 3)
        : ratePerDay_(ratePerDay), maxDaysLate_(maxDaysLate) {}
    int applyPenalty(int rawMark, int maxMark, int daysLate) const override;
    std::string describe() const override;
private:
    double ratePerDay_;
    int maxDaysLate_;
};

class NoPenaltyPolicy : public LatePenaltyPolicy {
public:
    int applyPenalty(int rawMark, int, int) const override { return rawMark; }
    std::string describe() const override { return "No late penalty"; }
};

}  // namespace lms
