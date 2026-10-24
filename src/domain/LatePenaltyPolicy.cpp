#include "LatePenaltyPolicy.h"

#include <algorithm>
#include <cmath>

namespace lms {

int TenPercentPerDayPolicy::applyPenalty(int rawMark, int maxMark, int daysLate) const {
    if (daysLate <= 0) return rawMark;
    if (daysLate > maxDaysLate_) return 0;
    const int deduction = static_cast<int>(std::lround(maxMark * ratePerDay_ * daysLate));
    return std::max(0, rawMark - deduction);
}

std::string TenPercentPerDayPolicy::describe() const {
    return std::to_string(static_cast<int>(std::lround(ratePerDay_ * 100))) + "% of the available mark per day late, up to " +
           std::to_string(maxDaysLate_) + " days; zero after that";
}

}  // namespace lms
