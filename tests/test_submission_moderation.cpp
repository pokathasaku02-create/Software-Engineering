// UNIT / INTEGRATION tests of the core business rules: submission, late penalty, extension,
// resubmission, marking, moderation, release and the audit log.
// Reference times: deadline = 2026-10-10 12:00, extension (where used) = 2026-10-13 12:00.
#include "test_support.h"

using namespace lms;
using namespace lms::test;

class SubmissionTest : public LmsTest {
protected:
    void giveExtension(const char* until = "2026-10-13 12:00") {
        ExtensionData e;
        e.assignmentId = w.assignmentId; e.studentId = w.student1->getId(); e.newDeadline = T(until); e.reason = "illness";
        w.assignmentSvc.grantExtension(*w.lecturer, e);
    }
};

// ------------------------------------------------------------------ submit: equivalence classes + boundaries

TEST_F(SubmissionTest, OnTimeBoundaryValues) {
    struct Row { const char* when; SubmissionStatus expected; };
    const Row rows[] = {{"2026-10-05 10:00", SubmissionStatus::Submitted},   // well before
                        {"2026-10-10 11:59:59", SubmissionStatus::Submitted}, // 1 s before
                        {"2026-10-10 12:00:00", SubmissionStatus::Submitted}, // exactly at the deadline is on time
                        {"2026-10-10 12:00:01", SubmissionStatus::Late},      // 1 s late
                        {"2026-10-11 12:00:00", SubmissionStatus::Late},
                        {"2026-10-20 12:00:00", SubmissionStatus::Late}};     // far too late: still accepted
    for (const Row& r : rows) {
        World fresh;
        fresh.setUpStandard();
        fresh.clock.set(T(r.when));
        Receipt rc = fresh.submissionSvc.submit(*fresh.student1, fresh.assignmentId, "work");
        EXPECT_EQ(rc.status, r.expected) << r.when;
        EXPECT_FALSE(rc.resubmission);
        EXPECT_EQ(rc.submittedAt, T(r.when));
    }
}

TEST_F(SubmissionTest, LateWorkIsAcceptedNeverRejected) {
    Receipt r = submitAt("2026-11-30 12:00", *w.student1);
    EXPECT_EQ(r.status, SubmissionStatus::Late);
    EXPECT_TRUE(load(r.submissionId).has_value());
}

TEST_F(SubmissionTest, ExtensionMovesTheDeadlineForThatStudentOnly) {
    giveExtension();
    EXPECT_EQ(submitAt("2026-10-12 08:00", *w.student1).status, SubmissionStatus::Submitted);   // extended
    EXPECT_EQ(submitAt("2026-10-12 08:00", *w.student2).status, SubmissionStatus::Late);        // no extension
}

TEST_F(SubmissionTest, ExtensionBoundaryValues) {
    giveExtension();
    World& x = w;
    x.clock.set(T("2026-10-13 12:00:00"));
    EXPECT_EQ(x.submissionSvc.submit(*x.student1, x.assignmentId, "a").status, SubmissionStatus::Submitted);
    World fresh; fresh.setUpStandard();
    ExtensionData e; e.assignmentId = fresh.assignmentId; e.studentId = fresh.student1->getId();
    e.newDeadline = T("2026-10-13 12:00"); e.reason = "r";
    fresh.assignmentSvc.grantExtension(*fresh.lecturer, e);
    fresh.clock.set(T("2026-10-13 12:00:01"));
    EXPECT_EQ(fresh.submissionSvc.submit(*fresh.student1, fresh.assignmentId, "a").status, SubmissionStatus::Late);
}

TEST_F(SubmissionTest, ResubmissionBeforeTheDeadlineReplacesTheWork) {
    Receipt first = submitAt("2026-10-05 10:00", *w.student1, "version 1");
    Receipt second = submitAt("2026-10-06 10:00", *w.student1, "version 2");
    EXPECT_TRUE(second.resubmission);
    EXPECT_EQ(second.submissionId, first.submissionId);
    std::optional<Submission> s = load(first.submissionId);
    EXPECT_EQ(s->getContent(), std::string("version 2"));
    EXPECT_EQ(s->getResubmitCount(), 1);
    EXPECT_EQ(s->getStatus(), SubmissionStatus::Submitted);
    submitAt("2026-10-10 12:00", *w.student1, "version 3");     // exactly at the deadline still allowed
    EXPECT_EQ(load(first.submissionId)->getResubmitCount(), 2);
    Statement c = w.db.prepare("SELECT COUNT(*) FROM submission");
    c.step();
    EXPECT_EQ(c.columnInt(0), 1);                                // never a duplicate row
}

TEST_F(SubmissionTest, ResubmissionAfterTheDeadlineIsRefused) {
    submitAt("2026-10-05 10:00", *w.student1, "v1");
    EXPECT_THROW(submitAt("2026-10-10 12:00:01", *w.student1, "v2"), ResubmissionClosedException);
    EXPECT_EQ(load(1)->getContent(), std::string("v1"));
}

TEST_F(SubmissionTest, LateWorkCannotBeResubmitted) {
    submitAt("2026-10-11 10:00", *w.student1, "late v1");
    EXPECT_THROW(submitAt("2026-10-11 11:00", *w.student1, "late v2"), ResubmissionClosedException);
}

TEST_F(SubmissionTest, ResubmissionAfterMarkingIsRefused) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-06 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 70, "fine", "");
    EXPECT_THROW(submitAt("2026-10-07 10:00", *w.student1, "oops"), IllegalStateException);
}

// Regression test for DEF-002: late first, extension granted afterwards, then resubmit within the extension.
TEST_F(SubmissionTest, LateSubmissionCanBeResubmittedOnceAnExtensionCoversTheTime) {
    Receipt first = submitAt("2026-10-11 10:00", *w.student1, "late v1");
    EXPECT_EQ(first.status, SubmissionStatus::Late);
    at("2026-10-11 11:00");
    giveExtension();
    Receipt second = submitAt("2026-10-12 10:00", *w.student1, "late v2");
    EXPECT_TRUE(second.resubmission);
    EXPECT_EQ(second.status, SubmissionStatus::Submitted);
    EXPECT_EQ(load(first.submissionId)->getContent(), std::string("late v2"));
    EXPECT_THROW(submitAt("2026-10-13 12:00:01", *w.student1, "too late now"), ResubmissionClosedException);
}

// Documented decision: an extension granted after a late submission also removes the penalty,
// because the penalty is always measured from the effective deadline.
TEST_F(SubmissionTest, ExtensionGrantedAfterALateSubmissionRemovesThePenalty) {
    Receipt r = submitAt("2026-10-11 10:00", *w.student1, "late");
    at("2026-10-11 11:00");
    giveExtension();
    at("2026-10-14 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 80, "ok", "");
    EXPECT_EQ(load(r.submissionId)->getFinalMark(), 80);
}

TEST_F(SubmissionTest, ExtensionAllowsResubmissionAfterTheOriginalDeadline) {
    giveExtension();
    submitAt("2026-10-09 10:00", *w.student1, "v1");
    EXPECT_NO_THROW(submitAt("2026-10-12 10:00", *w.student1, "v2"));
}

TEST_F(SubmissionTest, ContentLengthBoundaries) {
    EXPECT_THROW(submitAt("2026-10-05 10:00", *w.student1, ""), ValidationException);
    EXPECT_NO_THROW(submitAt("2026-10-05 10:00", *w.student1, "x"));
    World fresh; fresh.setUpStandard();
    fresh.clock.set(T("2026-10-05 10:00"));
    EXPECT_NO_THROW(fresh.submissionSvc.submit(*fresh.student1, fresh.assignmentId, std::string(20000, 'a')));
    EXPECT_THROW(fresh.submissionSvc.submit(*fresh.student2, fresh.assignmentId, std::string(20001, 'a')), ValidationException);
}

TEST_F(SubmissionTest, AccessRulesForSubmitting) {
    EXPECT_THROW(w.submissionSvc.submit(*w.lecturer, w.assignmentId, "x"), AccessDeniedException);
    EXPECT_THROW(w.submissionSvc.submit(*w.hod, w.assignmentId, "x"), AccessDeniedException);
    EXPECT_THROW(w.submissionSvc.submit(*w.admin, w.assignmentId, "x"), AccessDeniedException);
    EXPECT_THROW(w.submissionSvc.submit(*w.student1, 9999, "x"), NotFoundException);
    std::unique_ptr<User> stranger = w.addUser(Role::Student, "stranger");
    EXPECT_THROW(w.submissionSvc.submit(*stranger, w.assignmentId, "x"), AccessDeniedException);   // not enrolled
    std::unique_ptr<User> off = w.userRepo.findById(w.student2->getId());
    off->setActive(false);
    EXPECT_THROW(w.submissionSvc.submit(*off, w.assignmentId, "x"), AccessDeniedException);
}

TEST_F(SubmissionTest, StatusForReturnsOwnSubmissionOnly) {
    EXPECT_FALSE(w.submissionSvc.statusFor(*w.student1, w.assignmentId).has_value());
    submitAt("2026-10-05 10:00", *w.student1);
    EXPECT_TRUE(w.submissionSvc.statusFor(*w.student1, w.assignmentId).has_value());
    EXPECT_FALSE(w.submissionSvc.statusFor(*w.student2, w.assignmentId).has_value());
    EXPECT_THROW(w.submissionSvc.statusFor(*w.lecturer, w.assignmentId), AccessDeniedException);
}

// ------------------------------------------------------------------ marking and the late penalty

TEST_F(SubmissionTest, OnTimeWorkKeepsTheLecturersMark) {
    Receipt r = submitAt("2026-10-10 12:00", *w.student1);
    at("2026-10-12 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 80, "Good work", "");
    std::optional<Submission> s = load(r.submissionId);
    EXPECT_EQ(s->getRawMark(), 80);
    EXPECT_EQ(s->getFinalMark(), 80);
    EXPECT_EQ(s->getStatus(), SubmissionStatus::Marked);
    EXPECT_EQ(s->getFeedback(), std::string("Good work"));
}

TEST_F(SubmissionTest, LatePenaltyBoundaryValues) {
    struct Row { const char* submitted; int expectedFinal; };
    const Row rows[] = {{"2026-10-10 12:00:01", 70},   // 1 s late = 1 day late
                        {"2026-10-11 12:00:00", 70},   // exactly 1 day late
                        {"2026-10-11 12:00:01", 60},   // 2 days
                        {"2026-10-12 12:00:00", 60},
                        {"2026-10-12 12:00:01", 50},   // 3 days
                        {"2026-10-13 12:00:00", 50},   // exactly 3 days: still allowed
                        {"2026-10-13 12:00:01", 0},    // more than 3 days: zero
                        {"2026-11-30 12:00:00", 0}};
    for (const Row& r : rows) {
        World fresh; fresh.setUpStandard();
        fresh.clock.set(T(r.submitted));
        Receipt rc = fresh.submissionSvc.submit(*fresh.student1, fresh.assignmentId, "late");
        fresh.clock.set(T("2026-12-01 09:00"));
        fresh.submissionSvc.markSubmission(*fresh.lecturer, rc.submissionId, 80, "feedback", "");
        EXPECT_EQ(fresh.submissionRepo.findById(rc.submissionId)->getFinalMark(), r.expectedFinal) << r.submitted;
        EXPECT_EQ(fresh.submissionRepo.findById(rc.submissionId)->getRawMark(), 80) << "raw mark must stay unchanged";
    }
}

TEST_F(SubmissionTest, PenaltyIsMeasuredFromTheExtendedDeadline) {
    giveExtension();                                    // new deadline 2026-10-13 12:00
    Receipt r = submitAt("2026-10-14 10:00", *w.student1);   // 1 day after the extension
    EXPECT_EQ(r.status, SubmissionStatus::Late);
    at("2026-10-15 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 80, "ok", "");
    EXPECT_EQ(load(r.submissionId)->getFinalMark(), 70);
}

TEST_F(SubmissionTest, WorkSubmittedWithinTheExtensionIsNotPenalised) {
    giveExtension();
    Receipt r = submitAt("2026-10-12 10:00", *w.student1);
    at("2026-10-14 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 85, "ok", "");
    EXPECT_EQ(load(r.submissionId)->getFinalMark(), 85);
}

TEST_F(SubmissionTest, MarkRangeBoundaries) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, -1, "f", ""), ValidationException);
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 101, "f", ""), ValidationException);
    EXPECT_NO_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 0, "f", ""));
    EXPECT_NO_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 100, "f", "changed my mind"));
}

TEST_F(SubmissionTest, WrittenFeedbackIsRequired) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 50, "", ""), ValidationException);
    EXPECT_NO_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 50, std::string(2000, 'f'), ""));
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 50, std::string(2001, 'f'), "why"), ValidationException);
}

TEST_F(SubmissionTest, ChangingAnExistingMarkNeedsAReason) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 60, "ok", "");
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 65, "better", ""), ValidationException);
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 65, "better", std::string(501, 'r')), ValidationException);
    EXPECT_NO_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 65, "better", std::string(500, 'r')));
    EXPECT_EQ(load(r.submissionId)->getFinalMark(), 65);
}

TEST_F(SubmissionTest, OnlyTheLecturerOfTheCourseMayMark) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    std::unique_ptr<User> other = w.addUser(Role::Lecturer, "other_lec", w.dept);
    EXPECT_THROW(w.submissionSvc.markSubmission(*other, r.submissionId, 50, "f", ""), AccessDeniedException);
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.student1, r.submissionId, 100, "f", ""), AccessDeniedException);
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.hod, r.submissionId, 50, "f", ""), AccessDeniedException);
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, 9999, 50, "f", ""), NotFoundException);
    EXPECT_FALSE(load(r.submissionId)->getFinalMark().has_value());
}

TEST_F(SubmissionTest, MarkingIsNotPossibleOnceModerated) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 60, "ok", "");
    w.moderationSvc.approve(*w.hod, r.submissionId);
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 99, "f", "reason"), IllegalStateException);
    EXPECT_EQ(load(r.submissionId)->getFinalMark(), 60);
}

TEST_F(SubmissionTest, SubmissionsToMarkListsMarkableOnes) {
    Receipt a = submitAt("2026-10-05 10:00", *w.student1);
    submitAt("2026-10-11 10:00", *w.student2);
    EXPECT_EQ(w.submissionSvc.submissionsToMark(*w.lecturer, w.course).size(), 2u);
    w.submissionSvc.markSubmission(*w.lecturer, a.submissionId, 50, "f", "");
    EXPECT_EQ(w.submissionSvc.submissionsToMark(*w.lecturer, w.course).size(), 2u);   // marked ones can still be re-marked
    w.moderationSvc.approve(*w.hod, a.submissionId);
    EXPECT_EQ(w.submissionSvc.submissionsToMark(*w.lecturer, w.course).size(), 1u);   // moderated ones cannot
    EXPECT_THROW(w.submissionSvc.submissionsToMark(*w.student1, w.course), AccessDeniedException);
    EXPECT_THROW(w.submissionSvc.submissionsToMark(*w.lecturer, 9999), NotFoundException);
    std::unique_ptr<User> other = w.addUser(Role::Lecturer, "other_lec", w.dept);
    EXPECT_THROW(w.submissionSvc.submissionsToMark(*other, w.course), AccessDeniedException);
}

TEST_F(SubmissionTest, MarkAndAuditEntryAreAtomic) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.db.exec("DROP TRIGGER audit_no_update; DROP TRIGGER audit_no_delete; DROP TABLE audit_entry;");   // make the audit write fail
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 50, "f", ""), DatabaseException);
    std::optional<Submission> s = load(r.submissionId);
    EXPECT_EQ(s->getStatus(), SubmissionStatus::Submitted);          // the mark was rolled back too
    EXPECT_FALSE(s->getFinalMark().has_value());
}

// ------------------------------------------------------------------ audit trail of marking

TEST_F(SubmissionTest, EveryMarkChangeIsAuditedWithWhoWhenOldNewWhy) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 60, "ok", "");
    at("2026-10-11 10:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 65, "ok", "Second reading");
    std::vector<AuditEntry> h = w.audit.historyOf(*w.hod, r.submissionId);
    ASSERT_EQ(h.size(), 2u);
    EXPECT_EQ(h[0].getChangedBy(), w.lecturer->getId());
    EXPECT_EQ(h[0].getChangedAt(), T("2026-10-11 09:00"));
    EXPECT_FALSE(h[0].getOldValue().has_value());
    EXPECT_EQ(h[0].getNewValue(), 60);
    EXPECT_EQ(h[0].getReason(), std::string("Initial marking"));
    EXPECT_EQ(h[1].getOldValue(), 60);
    EXPECT_EQ(h[1].getNewValue(), 65);
    EXPECT_EQ(h[1].getReason(), std::string("Second reading"));
    EXPECT_EQ(h[1].getChangedAt(), T("2026-10-11 10:00"));
}

// ------------------------------------------------------------------ student visibility

TEST_F(SubmissionTest, StudentsSeeMarksOnlyAfterRelease) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 72, "Nice", "");
    EXPECT_EQ(w.submissionSvc.releasedMarksFor(*w.student1).size(), 0u);        // Marked
    w.moderationSvc.approve(*w.hod, r.submissionId);
    EXPECT_EQ(w.submissionSvc.releasedMarksFor(*w.student1).size(), 0u);        // Moderated
    EXPECT_EQ(w.moderationSvc.releaseApproved(*w.hod, w.course), 1);
    std::vector<Submission> seen = w.submissionSvc.releasedMarksFor(*w.student1);
    ASSERT_EQ(seen.size(), 1u);
    EXPECT_EQ(seen[0].getFinalMark(), 72);
    EXPECT_EQ(seen[0].getFeedback(), std::string("Nice"));
    EXPECT_EQ(w.submissionSvc.releasedMarksFor(*w.student2).size(), 0u);        // someone else's mark
    EXPECT_THROW(w.submissionSvc.releasedMarksFor(*w.lecturer), AccessDeniedException);
}

// ------------------------------------------------------------------ moderation

class ModerationTest : public LmsTest {
protected:
    int markedSubmission(User& student, int mark = 60) {
        Receipt r = submitAt("2026-10-05 10:00", student);
        at("2026-10-11 09:00");
        w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, mark, "feedback", "");
        return r.submissionId;
    }
    std::unique_ptr<User> otherDepartmentHod() {
        const int arts = w.courseSvc.createDepartment(*w.admin, "Arts");
        return w.addUser(Role::HeadOfDepartment, "arts_hod", arts);
    }
};

TEST_F(ModerationTest, HodSeesMarkedSubmissionsOfOwnDepartment) {
    markedSubmission(*w.student1);
    EXPECT_EQ(w.moderationSvc.marksForReview(*w.hod, w.course).size(), 1u);
}

TEST_F(ModerationTest, OtherActorsAndOtherDepartmentsAreRefused) {
    markedSubmission(*w.student1);
    EXPECT_THROW(w.moderationSvc.marksForReview(*w.lecturer, w.course), AccessDeniedException);
    EXPECT_THROW(w.moderationSvc.marksForReview(*w.student1, w.course), AccessDeniedException);
    EXPECT_THROW(w.moderationSvc.marksForReview(*w.admin, w.course), AccessDeniedException);
    std::unique_ptr<User> artsHod = otherDepartmentHod();
    EXPECT_THROW(w.moderationSvc.marksForReview(*artsHod, w.course), AccessDeniedException);
    EXPECT_THROW(w.moderationSvc.approve(*artsHod, 1), AccessDeniedException);
    EXPECT_THROW(w.moderationSvc.adjustMark(*artsHod, 1, 50, "x"), AccessDeniedException);
    EXPECT_THROW(w.moderationSvc.releaseApproved(*artsHod, w.course), AccessDeniedException);
    EXPECT_THROW(w.moderationSvc.marksForReview(*w.hod, 9999), NotFoundException);
    EXPECT_THROW(w.moderationSvc.approve(*w.hod, 9999), NotFoundException);
}

TEST_F(ModerationTest, AdjustMarkChangesFinalMarkKeepsRawAndIsAudited) {
    const int id = markedSubmission(*w.student1, 60);
    at("2026-10-12 10:00");
    w.moderationSvc.adjustMark(*w.hod, id, 68, "Rubric re-check");
    std::optional<Submission> s = load(id);
    EXPECT_EQ(s->getFinalMark(), 68);
    EXPECT_EQ(s->getRawMark(), 60);
    EXPECT_EQ(s->getStatus(), SubmissionStatus::Marked);
    std::vector<AuditEntry> h = w.audit.historyOf(*w.hod, id);
    ASSERT_EQ(h.size(), 2u);
    EXPECT_EQ(h[1].getChangedBy(), w.hod->getId());
    EXPECT_EQ(h[1].getOldValue(), 60);
    EXPECT_EQ(h[1].getNewValue(), 68);
    EXPECT_EQ(h[1].getReason(), std::string("Rubric re-check"));
    EXPECT_EQ(h[1].getChangedAt(), T("2026-10-12 10:00"));
}

TEST_F(ModerationTest, AdjustMarkValidation) {
    const int id = markedSubmission(*w.student1);
    EXPECT_THROW(w.moderationSvc.adjustMark(*w.hod, id, 50, ""), ValidationException);
    EXPECT_THROW(w.moderationSvc.adjustMark(*w.hod, id, 50, std::string(501, 'r')), ValidationException);
    EXPECT_THROW(w.moderationSvc.adjustMark(*w.hod, id, -1, "r"), ValidationException);
    EXPECT_THROW(w.moderationSvc.adjustMark(*w.hod, id, 101, "r"), ValidationException);
    EXPECT_NO_THROW(w.moderationSvc.adjustMark(*w.hod, id, 0, "r"));
    EXPECT_NO_THROW(w.moderationSvc.adjustMark(*w.hod, id, 100, std::string(500, 'r')));
    EXPECT_EQ(load(id)->getFinalMark(), 100);
}

TEST_F(ModerationTest, AdjustOrApproveOnlyWhileMarked) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    EXPECT_THROW(w.moderationSvc.approve(*w.hod, r.submissionId), IllegalStateException);              // still Submitted
    EXPECT_THROW(w.moderationSvc.adjustMark(*w.hod, r.submissionId, 5, "r"), IllegalStateException);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 60, "f", "");
    w.moderationSvc.approve(*w.hod, r.submissionId);
    EXPECT_EQ(load(r.submissionId)->getStatus(), SubmissionStatus::Moderated);
    EXPECT_THROW(w.moderationSvc.approve(*w.hod, r.submissionId), IllegalStateException);              // twice
    EXPECT_THROW(w.moderationSvc.adjustMark(*w.hod, r.submissionId, 5, "r"), IllegalStateException);   // too late to change
}

TEST_F(ModerationTest, ReleaseOnlyAffectsApprovedMarks) {
    const int a = markedSubmission(*w.student1);
    Receipt b = submitAt("2026-10-05 11:00", *w.student2);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, b.submissionId, 55, "f", "");
    w.moderationSvc.approve(*w.hod, a);                                // only student1's mark is approved
    EXPECT_EQ(w.moderationSvc.releaseApproved(*w.hod, w.course), 1);
    EXPECT_EQ(load(a)->getStatus(), SubmissionStatus::Released);
    EXPECT_EQ(load(b.submissionId)->getStatus(), SubmissionStatus::Marked);    // not approved: not released
    EXPECT_EQ(w.moderationSvc.releaseApproved(*w.hod, w.course), 0);           // nothing left to release
    EXPECT_EQ(w.submissionSvc.releasedMarksFor(*w.student2).size(), 0u);
}

TEST_F(ModerationTest, ReleaseNeedsTheReleasePermission) {
    EXPECT_THROW(w.moderationSvc.releaseApproved(*w.lecturer, w.course), AccessDeniedException);
    EXPECT_THROW(w.moderationSvc.releaseApproved(*w.hod, 9999), NotFoundException);
}

// ------------------------------------------------------------------ audit service

TEST_F(ModerationTest, AuditHistoryIsRestrictedToHodAndAdministrator) {
    const int id = markedSubmission(*w.student1);
    EXPECT_EQ(w.audit.historyOf(*w.hod, id).size(), 1u);
    EXPECT_EQ(w.audit.historyOf(*w.admin, id).size(), 1u);
    EXPECT_THROW(w.audit.historyOf(*w.student1, id), AccessDeniedException);
    EXPECT_THROW(w.audit.historyOf(*w.lecturer, id), AccessDeniedException);
    std::unique_ptr<User> off = w.userRepo.findById(w.hod->getId());
    off->setActive(false);
    EXPECT_THROW(w.audit.historyOf(*off, id), AccessDeniedException);
}

TEST_F(ModerationTest, AuditNeedsAReason) {
    const int id = markedSubmission(*w.student1);
    EXPECT_THROW(w.audit.logMarkChange(w.hod->getId(), id, 60, 61, ""), ValidationException);
    EXPECT_NO_THROW(w.audit.logMarkChange(w.hod->getId(), id, 60, 61, "x"));
}
