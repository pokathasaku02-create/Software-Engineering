#include "AuditService.h"

#include "../domain/Exceptions.h"

namespace lms {

void AuditService::logMarkChange(int userId, int submissionId, std::optional<int> oldMark, int newMark,
                                 const std::string& reason) {
    if (reason.empty()) throw ValidationException("A reason is required for every mark change.");
    audits_.save(AuditEntry(0, submissionId, userId, clock_.now(), oldMark, newMark, reason));
}

std::vector<AuditEntry> AuditService::historyOf(const User& actor, int submissionId) {
    if (!actor.isActive() ||
        (actor.getRole() != Role::HeadOfDepartment && actor.getRole() != Role::Administrator))
        throw AccessDeniedException("Only a head of department or an administrator may read the audit log.");
    return audits_.findBySubmission(submissionId);
}

}  // namespace lms
