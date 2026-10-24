#include "ModerationService.h"

#include "Authorization.h"

namespace lms {

Course ModerationService::requireOwnDepartment(const User& hod, int courseId) {
    requirePermission(hod, Permission::ModerateMarks, "moderate marks");
    std::optional<Course> course = courses_.findById(courseId);
    if (!course) throw NotFoundException("No such course.");
    if (hod.getDepartmentId() != course->getDepartmentId())
        throw AccessDeniedException("That course is not in your department.");
    return *course;
}

Submission ModerationService::loadForModeration(const User& hod, int submissionId, Assignment& assignmentOut) {
    requirePermission(hod, Permission::ModerateMarks, "moderate marks");
    std::optional<Submission> sub = submissions_.findById(submissionId);
    if (!sub) throw NotFoundException("No such submission.");
    std::optional<Assignment> a = assignments_.findById(sub->getAssignmentId());
    if (!a) throw NotFoundException("The assignment of that submission no longer exists.");
    requireOwnDepartment(hod, a->getCourseId());
    assignmentOut = *a;
    return *sub;
}

std::vector<Submission> ModerationService::marksForReview(const User& hod, int courseId) {
    requireOwnDepartment(hod, courseId);
    return submissions_.findByCourseAndStatus(courseId, SubmissionStatus::Marked);
}

void ModerationService::adjustMark(const User& hod, int submissionId, int newMark, const std::string& reason) {
    Assignment a(0, 0, "", "", 0, 1);
    Submission sub = loadForModeration(hod, submissionId, a);
    if (reason.empty() || reason.size() > 500) throw ValidationException("A reason of 1 to 500 characters is required.");
    if (newMark < 0 || newMark > a.getMaxMark())
        throw ValidationException("The mark must be between 0 and " + std::to_string(a.getMaxMark()) + ".");
    const std::optional<int> oldMark = sub.getFinalMark();
    sub.adjustFinalMark(newMark);                           // throws IllegalStateException unless MARKED

    ScopedTransaction tx(tx_);
    submissions_.update(sub);
    audit_.logMarkChange(hod.getId(), submissionId, oldMark, newMark, reason);
    tx.commit();
}

void ModerationService::approve(const User& hod, int submissionId) {
    Assignment a(0, 0, "", "", 0, 1);
    Submission sub = loadForModeration(hod, submissionId, a);
    sub.approve();
    submissions_.update(sub);
}

int ModerationService::releaseApproved(const User& hod, int courseId) {
    requireOwnDepartment(hod, courseId);
    requirePermission(hod, Permission::ReleaseMarks, "release marks");
    ScopedTransaction tx(tx_);
    int released = 0;
    for (Submission s : submissions_.findByCourseAndStatus(courseId, SubmissionStatus::Moderated)) {
        s.release();
        submissions_.update(s);
        ++released;
    }
    tx.commit();
    return released;
}

}  // namespace lms
