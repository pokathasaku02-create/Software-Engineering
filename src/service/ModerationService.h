#pragma once
#include <string>
#include <vector>

#include "../data/ITransactionManager.h"
#include "../data/Repositories.h"
#include "AuditService.h"

namespace lms {

class ModerationService {
public:
    ModerationService(ICourseRepository& courses, ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                      AuditService& audit, ITransactionManager& tx)
        : courses_(courses), submissions_(submissions), assignments_(assignments), audit_(audit), tx_(tx) {}

    // MARKED submissions of a course in the head's own department.
    std::vector<Submission> marksForReview(const User& hod, int courseId);
    // Changes a mark during moderation. The reason is mandatory and the change is audited.
    void adjustMark(const User& hod, int submissionId, int newMark, const std::string& reason);
    // MARKED -> MODERATED.
    void approve(const User& hod, int submissionId);
    // MODERATED -> RELEASED for the whole course; returns how many marks were released.
    int releaseApproved(const User& hod, int courseId);

private:
    Course requireOwnDepartment(const User& hod, int courseId);
    Submission loadForModeration(const User& hod, int submissionId, Assignment& assignmentOut);

    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    AuditService& audit_;
    ITransactionManager& tx_;
};

}  // namespace lms
