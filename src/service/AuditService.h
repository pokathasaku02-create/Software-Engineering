#pragma once
#include <string>
#include <vector>

#include "../data/Repositories.h"
#include "../domain/IClock.h"

namespace lms {

class AuditService {
public:
    AuditService(IAuditRepository& audits, IClock& clock) : audits_(audits), clock_(clock) {}
    // Records who changed a mark, when, from what to what, and why. The reason is mandatory.
    void logMarkChange(int userId, int submissionId, std::optional<int> oldMark, int newMark, const std::string& reason);
    // Only a Head of Department or an Administrator may read the audit history.
    std::vector<AuditEntry> historyOf(const User& actor, int submissionId);
private:
    IAuditRepository& audits_;
    IClock& clock_;
};

}  // namespace lms
