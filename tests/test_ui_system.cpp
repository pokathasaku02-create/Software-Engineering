// UNIT tests of InputValidator, and SYSTEM + ACCEPTANCE tests that drive the real menus through a
// scripted console. System tests follow the use-case flows (main, alternative, exception);
// acceptance tests check each stakeholder statement from the case study.
#include <chrono>

#include "test_support.h"

using namespace lms;
using namespace lms::test;

// ------------------------------------------------------------------ InputValidator (equivalence + boundaries)

TEST(InputValidator, TextTrimsAndEnforcesLength) {
    EXPECT_EQ(InputValidator::text("  hello  ", "x", 1, 10), std::string("hello"));
    EXPECT_EQ(InputValidator::text("a\tb\x01" "c", "x", 1, 10), std::string("abc"));       // control characters removed
    EXPECT_THROW(InputValidator::text("", "x", 1, 10), ValidationException);
    EXPECT_THROW(InputValidator::text("   ", "x", 1, 10), ValidationException);
    EXPECT_NO_THROW(InputValidator::text(std::string(10, 'a'), "x", 1, 10));
    EXPECT_THROW(InputValidator::text(std::string(11, 'a'), "x", 1, 10), ValidationException);
    EXPECT_EQ(InputValidator::text("", "x", 0, 10), std::string(""));                      // empty allowed when min is 0
}

TEST(InputValidator, IntegerParsingAndRange) {
    EXPECT_EQ(InputValidator::integer("42", "n", 0, 100), 42);
    EXPECT_EQ(InputValidator::integer("  7 ", "n", 0, 100), 7);
    EXPECT_EQ(InputValidator::integer("+5", "n", 0, 100), 5);
    EXPECT_EQ(InputValidator::integer("-3", "n", -10, 10), -3);
    EXPECT_EQ(InputValidator::integer("0", "n", 0, 100), 0);
    EXPECT_EQ(InputValidator::integer("100", "n", 0, 100), 100);
    EXPECT_THROW(InputValidator::integer("101", "n", 0, 100), ValidationException);
    EXPECT_THROW(InputValidator::integer("-1", "n", 0, 100), ValidationException);
    for (const char* bad : {"", " ", "abc", "1.5", "12abc", "--3", "+", "-", "1 2", "99999999999", "0x10"})
        EXPECT_THROW(InputValidator::integer(bad, "n", -1000, 1000), ValidationException) << bad;
}

TEST(InputValidator, DateTimeUsernameAndPassword) {
    EXPECT_EQ(InputValidator::dateTime(" 2026-10-10 12:00 "), parseDateTime("2026-10-10 12:00"));
    EXPECT_THROW(InputValidator::dateTime("10/10/2026"), ValidationException);
    EXPECT_EQ(InputValidator::username("  alice_1 "), std::string("alice_1"));
    EXPECT_THROW(InputValidator::username("a b"), ValidationException);
    EXPECT_EQ(InputValidator::password("  eight  "), std::string("  eight  "));            // spaces are kept in passwords
    EXPECT_THROW(InputValidator::password("short"), ValidationException);
}

// ------------------------------------------------------------------ system tests (use cases through the menus)

class SystemTest : public LmsTest {
protected:
    // Runs the log-in menu with the given input lines and returns the transcript.
    std::string run(std::vector<std::string> lines) {
        ScriptedConsole console(std::move(lines));
        LoginMenu menu(console, w.services());
        menu.run();
        last_ = console.transcript();
        return last_;
    }
    bool saw(const std::string& needle) const { return last_.find(needle) != std::string::npos; }
    std::string last_;
};

TEST(SystemFirstRun, EmptyDatabaseAsksForAnAdministrator) {
    World world;
    ScriptedConsole c({"ab", "x", "x", "root1", "Root Admin", "weak", "root1", "Root Admin", "rootpass1", "root1", "rootpass1", "0", ""});
    LoginMenu(c, world.services()).run();
    EXPECT_TRUE(c.printed("First run"));
    EXPECT_TRUE(c.printed("at least 8 characters"));          // weak password rejected, asked again
    EXPECT_TRUE(c.printed("Administrator account created"));
    EXPECT_TRUE(c.printed("Administrator menu"));              // then logged in as that administrator
    EXPECT_TRUE(world.auth.hasAnyUsers());
}

TEST_F(SystemTest, LoginWrongPasswordThenCorrect) {
    run({"student1", "badpassword", "student1", "password1", "0", ""});
    EXPECT_TRUE(saw("Invalid username or password"));
    EXPECT_TRUE(saw("Student menu"));
}

TEST_F(SystemTest, LogoutReturnsToLoginAndEmptyUsernameExits) {
    run({"student1", "password1", "0", ""});
    EXPECT_TRUE(saw("log in (leave the username empty to exit)"));
}

TEST_F(SystemTest, ClosedInputEndsTheProgramGracefully) {
    run({"student1", "password1", "2"});          // input ends in the middle of a session
    EXPECT_TRUE(saw("Goodbye"));
}

// --- Use case: Submit assignment ---

TEST_F(SystemTest, SubmitAssignmentMainFlow) {
    at("2026-10-05 10:00");
    run({"student1", "password1", "3", "1", "my essay", "0", ""});
    EXPECT_TRUE(saw("Submitted at 2026-10-05 10:00:00"));
    EXPECT_TRUE(saw("Status: SUBMITTED"));
}

TEST_F(SystemTest, SubmitAssignmentAlternativeFlowResubmission) {
    at("2026-10-05 10:00");
    run({"student1", "password1", "3", "1", "draft", "3", "1", "final", "2", "0", ""});
    EXPECT_TRUE(saw("Resubmitted at"));
    EXPECT_EQ(w.submissionRepo.findByStudentAndAssignment(w.student1->getId(), w.assignmentId)->getContent(), std::string("final"));
}

TEST_F(SystemTest, SubmitAssignmentAlternativeFlowLateWorkIsAcceptedAndFlagged) {
    at("2026-10-11 10:00");
    run({"student1", "password1", "3", "1", "late essay", "0", ""});
    EXPECT_TRUE(saw("Status: LATE"));
    EXPECT_TRUE(saw("late work loses marks"));
}

TEST_F(SystemTest, SubmitAssignmentExceptionFlowLateResubmissionRefused) {
    at("2026-10-05 10:00");
    run({"student1", "password1", "3", "1", "v1", "0", ""});
    at("2026-10-10 12:00:01");
    run({"student1", "password1", "3", "1", "v2", "0", ""});
    EXPECT_TRUE(saw("can no longer be resubmitted"));
}

TEST_F(SystemTest, SubmitAssignmentExceptionFlowInvalidInputKeepsTheProgramRunning) {
    run({"student1", "password1", "3", "abc", "3", "-5", "3", "1", "", "99", "0", ""});
    EXPECT_TRUE(saw("must be a whole number"));
    EXPECT_TRUE(saw("must be between 0 and 4"));   // menu choice 99
    EXPECT_TRUE(saw("Student menu"));               // still alive
    EXPECT_FALSE(w.submissionRepo.findByStudentAndAssignment(w.student1->getId(), w.assignmentId).has_value());
}

TEST_F(SystemTest, SubmitAssignmentExceptionFlowNotEnrolledAndUnknownAssignment) {
    w.addUser(Role::Student, "outsider");
    run({"outsider", "password1", "3", "1", "x", "3", "999", "y", "0", ""});
    EXPECT_TRUE(saw("not enrolled"));
    EXPECT_TRUE(saw("No such assignment") || saw("not enrolled"));
}

TEST_F(SystemTest, SubmitAssignmentExceptionFlowDatabaseFailureDoesNotCrash) {
    w.db.exec("DROP TABLE audit_entry");           // unrelated table
    w.db.exec("PRAGMA foreign_keys = OFF; DROP TABLE submission;");
    run({"student1", "password1", "3", "1", "work", "1", "0", ""});
    EXPECT_TRUE(saw("database could not complete") || saw("unexpected error"));
    EXPECT_TRUE(saw("Student menu"));
    EXPECT_TRUE(saw("(no") || saw("Your courses") || saw("CS101"));   // later menu choice still worked
}

// --- Use case: Mark submission / Moderate and release marks ---

TEST_F(SystemTest, LecturerMarksThenStudentStillSeesNothing) {
    at("2026-10-05 10:00");
    run({"student1", "password1", "3", "1", "essay", "0", ""});
    at("2026-10-11 09:00");
    run({"lecturer1", "password1", "5", "1", "6", "1", "75", "Well argued", "", "0", ""});
    EXPECT_TRUE(saw("Mark saved"));
    run({"student1", "password1", "4", "0", ""});
    EXPECT_TRUE(saw("no marks have been released yet"));
}

TEST_F(SystemTest, FullModerationFlowThroughTheMenus) {
    at("2026-10-05 10:00");
    run({"student1", "password1", "3", "1", "essay", "0", ""});
    at("2026-10-11 09:00");
    run({"lecturer1", "password1", "6", "1", "75", "Well argued", "", "0", ""});
    run({"hod1", "password1", "2", "1", "3", "1", "70", "Rubric check", "5", "1", "6", "1", "7", "1", "0", ""});
    EXPECT_TRUE(saw("Mark adjusted"));
    EXPECT_TRUE(saw("1 mark(s) approved"));
    EXPECT_TRUE(saw("1 mark(s) released"));
    EXPECT_TRUE(saw("75 -> 70"));                               // audit history line (old -> new)
    EXPECT_TRUE(saw("Rubric check"));
    run({"student1", "password1", "4", "0", ""});
    EXPECT_TRUE(saw("Assignment 1: 70 / 100"));
    EXPECT_TRUE(saw("feedback: Well argued"));
}

TEST_F(SystemTest, HodOfAnotherDepartmentIsRefused) {
    const int arts = w.courseSvc.createDepartment(*w.admin, "Arts");
    w.addUser(Role::HeadOfDepartment, "arts_hod", arts);
    run({"arts_hod", "password1", "2", "1", "0", ""});
    EXPECT_TRUE(saw("not in your department"));
}

TEST_F(SystemTest, EachRoleSeesOnlyItsOwnMenu) {
    run({"student1", "password1", "0", ""});
    EXPECT_TRUE(saw("Submit or resubmit work"));
    EXPECT_FALSE(saw("Create a user account"));
    EXPECT_FALSE(saw("Release approved marks"));
    run({"lecturer1", "password1", "0", ""});
    EXPECT_TRUE(saw("Mark or re-mark a submission"));
    EXPECT_FALSE(saw("Submit or resubmit work"));
    run({"hod1", "password1", "0", ""});
    EXPECT_TRUE(saw("Release approved marks"));
    EXPECT_FALSE(saw("Create a user account"));
    run({"admin2", "password1", "0", ""});
    EXPECT_TRUE(saw("Create a user account"));
}

TEST_F(SystemTest, AdministratorCanSetUpCourseDataThroughTheMenu) {
    run({"admin2", "password1",
         "3", "Arts",
         "1", "student", "newstu", "New Student", "N77", "longpassword",
         "2", "7", "0", ""});
    EXPECT_TRUE(saw("Department #"));
    EXPECT_TRUE(saw("User #"));
    EXPECT_TRUE(w.userRepo.findByUsername("newstu") != nullptr);
    run({"admin2", "password1", "1", "wizard", "0", ""});
    EXPECT_TRUE(saw("must be student, lecturer, hod or admin"));
}

// Regression test for DEF-003: unknown department gives a friendly message, not "FOREIGN KEY constraint failed".
TEST_F(SystemTest, AdministratorGetsAFriendlyMessageForAnUnknownDepartment) {
    run({"admin2", "password1", "1", "lecturer", "lec9", "Lec Nine", "T99", "9999", "longpassword", "0", ""});
    EXPECT_TRUE(saw("No such department"));
    EXPECT_FALSE(saw("FOREIGN KEY"));
    EXPECT_TRUE(w.userRepo.findByUsername("lec9") == nullptr);
}

TEST_F(SystemTest, AdministratorCreatesCourseAssignsLecturerEnrolsStudentAndListsCourses) {
    std::unique_ptr<User> student3 = w.addUser(Role::Student, "student3");
    run({"admin2", "password1",
         "4", "PH101", "Physics", std::to_string(w.dept),
         "5", "2", std::to_string(w.lecturer->getId()),
         "6", "2", std::to_string(student3->getId()),
         "7", "8", "1", "0", ""});
    EXPECT_TRUE(saw("Course #2 created"));
    EXPECT_TRUE(saw("Lecturer assigned"));
    EXPECT_TRUE(saw("Student enrolled"));
    EXPECT_TRUE(saw("PH101"));
    EXPECT_TRUE(saw("no mark changes recorded"));
    EXPECT_EQ(w.courseRepo.findByStudent(student3->getId()).size(), 1u);
    EXPECT_EQ(w.courseRepo.findById(2)->getLecturerId(), w.lecturer->getId());
}

TEST_F(SystemTest, EveryRoleCanListItsCourses) {
    run({"student1", "password1", "1", "0", ""});
    EXPECT_TRUE(saw("CS101"));
    run({"lecturer1", "password1", "1", "0", ""});
    EXPECT_TRUE(saw("CS101"));
    run({"hod1", "password1", "1", "0", ""});
    EXPECT_TRUE(saw("CS101"));
}

TEST_F(SystemTest, HodApprovesASingleMarkThroughTheMenu) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 66, "fine", "");
    run({"hod1", "password1", "4", std::to_string(r.submissionId), "0", ""});
    EXPECT_TRUE(saw("Approved."));
    EXPECT_EQ(load(r.submissionId)->getStatus(), SubmissionStatus::Moderated);
}

TEST_F(SystemTest, LecturerCreatesAssignmentAndExtensionThroughTheMenu) {
    run({"lecturer1", "password1",
         "2", std::to_string(w.course), "Lab 2", "", "2026-12-01 10:00", "20",
         "4", std::to_string(w.assignmentId), std::to_string(w.student1->getId()), "2026-10-14 12:00", "family emergency",
         "3", std::to_string(w.course), "0", ""});
    EXPECT_TRUE(saw("created"));
    EXPECT_TRUE(saw("Extension granted"));
    EXPECT_TRUE(saw("Lab 2"));
    run({"lecturer1", "password1", "2", std::to_string(w.course), "Bad", "", "not a date", "0", ""});
    EXPECT_TRUE(saw("Date/time must look like"));
}

// ------------------------------------------------------------------ acceptance tests (stakeholder statements)

class AcceptanceTest : public LmsTest {};

TEST_F(AcceptanceTest, Principal_StudentsNeverSeeAMarkBeforeTheHodHasCheckedIt) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 80, "good", "");
    EXPECT_EQ(w.submissionSvc.releasedMarksFor(*w.student1).size(), 0u);
    w.moderationSvc.approve(*w.hod, r.submissionId);
    EXPECT_EQ(w.submissionSvc.releasedMarksFor(*w.student1).size(), 0u);
    w.moderationSvc.releaseApproved(*w.hod, w.course);
    EXPECT_EQ(w.submissionSvc.releasedMarksFor(*w.student1).size(), 1u);
}

TEST_F(AcceptanceTest, Lecturer_CanGiveOneStudentMoreTimeWithoutPenalty) {
    ExtensionData e;
    e.assignmentId = w.assignmentId; e.studentId = w.student1->getId(); e.newDeadline = T("2026-10-14 12:00"); e.reason = "bereavement";
    w.assignmentSvc.grantExtension(*w.lecturer, e);
    Receipt r1 = submitAt("2026-10-13 09:00", *w.student1);
    Receipt r2 = submitAt("2026-10-13 09:00", *w.student2);
    at("2026-10-16 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r1.submissionId, 80, "ok", "");
    w.submissionSvc.markSubmission(*w.lecturer, r2.submissionId, 80, "ok", "");
    EXPECT_EQ(load(r1.submissionId)->getFinalMark(), 80);    // extended student: no penalty
    EXPECT_EQ(load(r2.submissionId)->getFinalMark(), 50);    // 3 days late: -30
}

TEST_F(AcceptanceTest, Hod_TenPercentPerDayForThreeDaysThenZero) {
    const int days[] = {0, 1, 2, 3, 4};
    const int expected[] = {100, 90, 80, 70, 0};
    for (int i = 0; i < 5; ++i) {
        World world; world.setUpStandard();
        world.clock.set(T("2026-10-10 12:00") + (days[i] == 0 ? 0 : days[i] * kSecondsPerDay - 3600));   // within the day
        Receipt r = world.submissionSvc.submit(*world.student1, world.assignmentId, "w");
        world.clock.set(T("2026-11-01 09:00"));
        world.submissionSvc.markSubmission(*world.lecturer, r.submissionId, 100, "f", "");
        EXPECT_EQ(world.submissionRepo.findById(r.submissionId)->getFinalMark(), expected[i]) << "days late " << days[i];
    }
}

TEST_F(AcceptanceTest, Hod_SeesEveryMarkInTheDepartmentBeforeRelease) {
    Receipt a = submitAt("2026-10-05 10:00", *w.student1);
    Receipt b = submitAt("2026-10-12 10:00", *w.student2);
    at("2026-10-14 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, a.submissionId, 70, "f", "");
    w.submissionSvc.markSubmission(*w.lecturer, b.submissionId, 70, "f", "");
    EXPECT_EQ(w.moderationSvc.marksForReview(*w.hod, w.course).size(), 2u);
}

TEST_F(AcceptanceTest, StudentRep_ResubmitBeforeDeadlineLateNotRejectedFeedbackIsText) {
    submitAt("2026-10-05 10:00", *w.student1, "with a mistake");
    Receipt fixed = submitAt("2026-10-09 10:00", *w.student1, "corrected");
    EXPECT_EQ(load(fixed.submissionId)->getContent(), std::string("corrected"));
    Receipt late = submitAt("2026-10-11 10:00", *w.student2, "a bit late");
    EXPECT_EQ(late.status, SubmissionStatus::Late);
    at("2026-10-14 09:00");
    EXPECT_THROW(w.submissionSvc.markSubmission(*w.lecturer, late.submissionId, 70, "", ""), ValidationException);   // feedback required
    w.submissionSvc.markSubmission(*w.lecturer, late.submissionId, 70, "Written comments for the student", "");
    EXPECT_EQ(load(late.submissionId)->getFeedback(), std::string("Written comments for the student"));
}

TEST_F(AcceptanceTest, School_EveryMarkChangeIsRecordedWithWhoWhenOldNewWhy) {
    Receipt r = submitAt("2026-10-05 10:00", *w.student1);
    at("2026-10-11 09:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 60, "f", "");
    at("2026-10-11 10:00");
    w.submissionSvc.markSubmission(*w.lecturer, r.submissionId, 62, "f", "Typo in total");
    at("2026-10-12 10:00");
    w.moderationSvc.adjustMark(*w.hod, r.submissionId, 66, "Moderation");
    std::vector<AuditEntry> h = w.audit.historyOf(*w.admin, r.submissionId);
    ASSERT_EQ(h.size(), 3u);
    EXPECT_EQ(h[2].getChangedBy(), w.hod->getId());
    EXPECT_EQ(h[2].getOldValue(), 62);
    EXPECT_EQ(h[2].getNewValue(), 66);
    EXPECT_EQ(h[2].getReason(), std::string("Moderation"));
}

// NFR: "login completes within 2 seconds with 800 users stored" and "800 students respond quickly".
TEST(Performance, LoginWithin2SecondsWith800UsersStoredUsingProductionHashing) {
    World world;
    PasswordHasher production;                       // real Argon2id cost, as in main.cpp
    AuthService prodAuth(world.userRepo, production);
    prodAuth.createInitialAdministrator("admin", "Admin", "adminpass1");
    {
        ScopedTransaction tx(world.db);
        for (int i = 0; i < 800; ++i) {
            UserDetails d;
            d.username = "student" + std::to_string(i); d.fullName = "S" + std::to_string(i);
            d.referenceNumber = "N" + std::to_string(i); d.passwordHash = std::string(64, 'a'); d.salt = std::string(32, 'b');
            world.userRepo.save(*UserFactory::create(Role::Student, d));
        }
        tx.commit();
    }
    EXPECT_EQ(world.userRepo.findAll().size(), 801u);
    const auto t0 = std::chrono::steady_clock::now();
    const User* u = prodAuth.login("admin", "adminpass1");
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    EXPECT_TRUE(u != nullptr);
    EXPECT_LT(seconds, 2.0);
    std::cout << "    [measured] login with 801 users stored: " << seconds << " s\n";
}

TEST(Performance, ReviewingAndReleasing800SubmissionsIsFast) {
    World world;
    world.setUpStandard();
    {
        ScopedTransaction tx(world.db);
        for (int i = 0; i < 800; ++i) {
            UserDetails d;
            d.username = "bulk" + std::to_string(i); d.fullName = "B"; d.referenceNumber = "B" + std::to_string(i);
            d.passwordHash = std::string(64, 'a'); d.salt = std::string(32, 'b');
            const int id = world.userRepo.save(*UserFactory::create(Role::Student, d));
            world.courseRepo.enrol(Enrolment(id, world.course, 0));
            Submission s = Submission::create(world.assignmentId, id, "work", T("2026-10-05 10:00"), false);
            s.applyMark(70, 70, "f");
            world.submissionRepo.save(s);
        }
        tx.commit();
    }
    auto t0 = std::chrono::steady_clock::now();
    const size_t n = world.moderationSvc.marksForReview(*world.hod, world.course).size();
    double review = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    EXPECT_EQ(n, 800u);
    EXPECT_LT(review, 1.0);
    for (const Submission& s : world.moderationSvc.marksForReview(*world.hod, world.course)) world.moderationSvc.approve(*world.hod, s.getId());
    t0 = std::chrono::steady_clock::now();
    EXPECT_EQ(world.moderationSvc.releaseApproved(*world.hod, world.course), 800);
    double release = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    EXPECT_LT(release, 2.0);
    std::cout << "    [measured] review 800 marks: " << review << " s, release 800 marks: " << release << " s\n";
}
