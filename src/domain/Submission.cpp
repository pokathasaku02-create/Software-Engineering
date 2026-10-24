#include "Submission.h"

#include <utility>

#include "Exceptions.h"

namespace lms {

Submission::Submission(int id, int assignmentId, int studentId, std::string content, DateTime submittedAt,
                       int resubmitCount, SubmissionStatus status, std::optional<int> rawMark,
                       std::optional<int> finalMark, std::string feedback)
    : submissionId_(id), assignmentId_(assignmentId), studentId_(studentId), content_(std::move(content)),
      submittedAt_(submittedAt), resubmitCount_(resubmitCount), status_(status), rawMark_(rawMark),
      finalMark_(finalMark), feedback_(std::move(feedback)) {}

Submission Submission::create(int assignmentId, int studentId, std::string content, DateTime at, bool late) {
    Submission s(0, assignmentId, studentId, std::move(content), at, 0,
                 late ? SubmissionStatus::Late : SubmissionStatus::Submitted, std::nullopt, std::nullopt, "");
    return s;
}

void Submission::requireStatus(SubmissionStatus expected, const char* action) const {
    if (status_ != expected)
        throw IllegalStateException(std::string("Cannot ") + action + " a submission that is " + toString(status_) + ".");
}

void Submission::resubmit(const std::string& content, DateTime at) {
    requireStatus(SubmissionStatus::Submitted, "resubmit");
    content_ = content;
    submittedAt_ = at;
    ++resubmitCount_;
}

void Submission::applyMark(int rawMark, int finalMark, const std::string& feedback) {
    if (status_ != SubmissionStatus::Submitted && status_ != SubmissionStatus::Late &&
        status_ != SubmissionStatus::Marked)
        throw IllegalStateException("Cannot mark a submission that is " + toString(status_) + ".");
    rawMark_ = rawMark;
    finalMark_ = finalMark;
    feedback_ = feedback;
    status_ = SubmissionStatus::Marked;
}

void Submission::adjustFinalMark(int newFinalMark) {
    requireStatus(SubmissionStatus::Marked, "adjust the mark of");
    finalMark_ = newFinalMark;
}

void Submission::approve() {
    requireStatus(SubmissionStatus::Marked, "approve");
    status_ = SubmissionStatus::Moderated;
}

void Submission::release() {
    requireStatus(SubmissionStatus::Moderated, "release");
    status_ = SubmissionStatus::Released;
}

}  // namespace lms
