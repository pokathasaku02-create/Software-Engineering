// Submission owns its life cycle (see the state machine diagram). Status changes happen
// only through these methods; any illegal transition throws IllegalStateException.
#pragma once
#include <optional>
#include <string>

#include "Types.h"

namespace lms {

class Submission {
public:
    Submission(int id, int assignmentId, int studentId, std::string content, DateTime submittedAt,
               int resubmitCount, SubmissionStatus status, std::optional<int> rawMark,
               std::optional<int> finalMark, std::string feedback);

    // New submission: Submitted if on time, Late otherwise.
    static Submission create(int assignmentId, int studentId, std::string content, DateTime at, bool late);

    int getId() const { return submissionId_; }
    int getAssignmentId() const { return assignmentId_; }
    int getStudentId() const { return studentId_; }
    const std::string& getContent() const { return content_; }
    DateTime getSubmittedAt() const { return submittedAt_; }
    int getResubmitCount() const { return resubmitCount_; }
    SubmissionStatus getStatus() const { return status_; }
    std::optional<int> getRawMark() const { return rawMark_; }
    std::optional<int> getFinalMark() const { return finalMark_; }
    const std::string& getFeedback() const { return feedback_; }
    void setId(int id) { submissionId_ = id; }

    // Submitted -> Submitted: the student replaces the work before the deadline.
    // Late -> Submitted: same, when an extension made the work on time after all.
    void resubmit(const std::string& content, DateTime at);
    // Submitted/Late -> Marked, or Marked -> Marked (lecturer re-marks before moderation).
    void applyMark(int rawMark, int finalMark, const std::string& feedback);
    // Marked -> Marked: the Head of Department changes the mark during moderation.
    void adjustFinalMark(int newFinalMark);
    // Marked -> Moderated.
    void approve();
    // Moderated -> Released.
    void release();

private:
    void requireStatus(SubmissionStatus expected, const char* action) const;

    int submissionId_, assignmentId_, studentId_;
    std::string content_;
    DateTime submittedAt_;
    int resubmitCount_;
    SubmissionStatus status_;
    std::optional<int> rawMark_, finalMark_;
    std::string feedback_;
};

}  // namespace lms
