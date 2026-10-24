// Deadline arithmetic shared by the services (kept free of I/O so it is easy to test).
#pragma once
#include <algorithm>
#include <optional>

#include "../domain/Entities.h"

namespace lms {

// The assignment deadline, or the student's extension when it is later.
inline DateTime effectiveDeadline(const Assignment& a, const std::optional<Extension>& ext) {
    return ext ? std::max(a.getDeadline(), ext->getNewDeadline()) : a.getDeadline();
}

// Whole days late, rounded up: 1 second late = 1 day late. Zero when on time.
inline int daysLate(DateTime submittedAt, DateTime deadline) {
    if (submittedAt <= deadline) return 0;
    return static_cast<int>((submittedAt - deadline + kSecondsPerDay - 1) / kSecondsPerDay);
}

}  // namespace lms
