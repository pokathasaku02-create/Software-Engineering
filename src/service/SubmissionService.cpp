#include "SubmissionService.h"

#include "Authorization.h"
#include "Deadlines.h"

namespace lms {

Receipt SubmissionService::submit(const User& student, int assignmentId, const std::string& content) {
    requirePermission(student, Permission::SubmitWork, "submit work");
    if (content.empty() || content.size() > 20000) throw ValidationException("The work must have 1 to 20000 characters.");
    std::optional<Assignment> assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundException("No such assignment.");
    bool enrolled = false;
    for (const Course& c : courses_.findByStudent(student.getId())) enrolled = enrolled || c.getId() == assignment->getCourseId();
    if (!enrolled) throw AccessDeniedException("You are not enrolled in that course.");

    const DateTime deadline = effectiveDeadline(*assignment, assignments_.findExtension(student.getId(), assignmentId));
    const DateTime now = clock_.now();
    const bool late = now > deadline;
    std::optional<Submission> existing = submissions_.findByStudentAndAssignment(student.getId(), assignmentId);

    if (existing) {
        if (late) throw ResubmissionClosedException("The deadline has passed; the work can no longer be resubmitted.");
        existing->resubmit(content, now);          // throws IllegalStateException if already marked
        submissions_.update(*existing);
        return Receipt{existing->getId(), existing->getStatus(), now, true};
    }
    Submission created = Submission::create(assignmentId, student.getId(), content, now, late);
    const int id = submissions_.save(created);
    return Receipt{id, created.getStatus(), now, false};
}

std::optional<Submission> SubmissionService::statusFor(const User& student, int assignmentId) {
    requirePermission(student, Permission::SubmitWork, "view your submissions");
    return submissions_.findByStudentAndAssignment(student.getId(), assignmentId);
}

std::vector<Submission> SubmissionService::submissionsToMark(const User& lecturer, int courseId) {
    requirePermission(lecturer, Permission::MarkSubmission, "mark submissions");
    std::optional<Course> course = courses_.findById(courseId);
    if (!course) throw NotFoundException("No such course.");
    if (course->getLecturerId() != lecturer.getId()) throw AccessDeniedException("You do not teach that course.");
    std::vector<Submission> out;
    for (SubmissionStatus st : {SubmissionStatus::Submitted, SubmissionStatus::Late, SubmissionStatus::Marked})
        for (const Submission& s : submissions_.findByCourseAndStatus(courseId, st)) out.push_back(s);
    return out;
}

void SubmissionService::markSubmission(const User& lecturer, int submissionId, int rawMark,
                                       const std::string& feedback, const std::string& reason) {
    requirePermission(lecturer, Permission::MarkSubmission, "mark submissions");
    std::optional<Submission> sub = submissions_.findById(submissionId);
    if (!sub) throw NotFoundException("No such submission.");
    std::optional<Assignment> assignment = assignments_.findById(sub->getAssignmentId());
    if (!assignment) throw NotFoundException("The assignment of that submission no longer exists.");
    std::optional<Course> course = courses_.findById(assignment->getCourseId());
    if (!course || course->getLecturerId() != lecturer.getId()) throw AccessDeniedException("You do not teach that course.");

    if (rawMark < 0 || rawMark > assignment->getMaxMark())
        throw ValidationException("The mark must be between 0 and " + std::to_string(assignment->getMaxMark()) + ".");
    if (feedback.empty() || feedback.size() > 2000) throw ValidationException("Written feedback of 1 to 2000 characters is required.");
    const bool remark = sub->getStatus() == SubmissionStatus::Marked;
    if (remark && reason.empty()) throw ValidationException("Give a reason for changing an existing mark.");
    if (reason.size() > 500) throw ValidationException("The reason is too long (maximum 500 characters).");

    const DateTime deadline = effectiveDeadline(*assignment, assignments_.findExtension(sub->getStudentId(), assignment->getId()));
    const int late = daysLate(sub->getSubmittedAt(), deadline);
    const int finalMark = policy_.applyPenalty(rawMark, assignment->getMaxMark(), late);

    const std::optional<int> oldMark = sub->getFinalMark();
    sub->applyMark(rawMark, finalMark, feedback);          // throws IllegalStateException if not markable

    ScopedTransaction tx(tx_);                              // mark change and audit entry succeed or fail together
    submissions_.update(*sub);
    audit_.logMarkChange(lecturer.getId(), submissionId, oldMark, finalMark, remark ? reason : "Initial marking");
    tx.commit();
}

std::vector<Submission> SubmissionService::releasedMarksFor(const User& student) {
    requirePermission(student, Permission::ViewReleasedMarks, "view marks");
    return submissions_.findReleasedByStudent(student.getId());
}

}  // namespace lms
