#pragma once
#include <string>
#include <vector>

#include "../data/ITransactionManager.h"
#include "../data/Repositories.h"
#include "../domain/IClock.h"
#include "../domain/LatePenaltyPolicy.h"
#include "AuditService.h"

namespace lms {

struct Receipt {
    int submissionId = 0;
    SubmissionStatus status = SubmissionStatus::Submitted;
    DateTime submittedAt = 0;
    bool resubmission = false;
};

class SubmissionService {
public:
    SubmissionService(IAssignmentRepository& assignments, ISubmissionRepository& submissions, ICourseRepository& courses,
                      AuditService& audit, const LatePenaltyPolicy& policy, ITransactionManager& tx, IClock& clock)
        : assignments_(assignments), submissions_(submissions), courses_(courses), audit_(audit),
          policy_(policy), tx_(tx), clock_(clock) {}

    // First submission, or resubmission before the effective deadline. Late work is accepted
    // (status LATE), never rejected; a late *re*submission is refused.
    Receipt submit(const User& student, int assignmentId, const std::string& content);
    std::optional<Submission> statusFor(const User& student, int assignmentId);

    // Lecturer: submissions of one of their courses that can still be marked or re-marked.
    std::vector<Submission> submissionsToMark(const User& lecturer, int courseId);
    // Marks (or re-marks) a submission. The late penalty is applied here. A reason is required
    // when changing an existing mark; every change is written to the audit log in the same transaction.
    void markSubmission(const User& lecturer, int submissionId, int rawMark, const std::string& feedback,
                        const std::string& reason);

    // Student: only RELEASED marks are ever returned.
    std::vector<Submission> releasedMarksFor(const User& student);

private:
    IAssignmentRepository& assignments_;
    ISubmissionRepository& submissions_;
    ICourseRepository& courses_;
    AuditService& audit_;
    const LatePenaltyPolicy& policy_;
    ITransactionManager& tx_;
    IClock& clock_;
};

}  // namespace lms
