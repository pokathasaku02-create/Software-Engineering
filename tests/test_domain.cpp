// UNIT tests for the domain layer: dates, penalty policy (boundary values), deadlines,
// the Submission state machine, user permissions and the user factory.
#include "test_support.h"
#include "service/Deadlines.h"

using namespace lms;
using namespace lms::test;

// ------------------------------------------------------------------ Types: dates and enums

TEST(DateTime, FormatAndParseRoundTrip) {
    const char* samples[] = {"2026-10-10 12:00:00", "2026-01-01 00:00:00", "2026-12-31 23:59:59", "2024-02-29 12:30:15"};
    for (const char* s : samples) EXPECT_EQ(formatDateTime(parseDateTime(s)), std::string(s));
}

TEST(DateTime, AcceptsMinutePrecisionAndAddsZeroSeconds) {
    EXPECT_EQ(formatDateTime(parseDateTime("2026-10-10 12:00")), std::string("2026-10-10 12:00:00"));
}

TEST(DateTime, OneDayIsExactly86400Seconds) {
    EXPECT_EQ(parseDateTime("2026-10-11 12:00") - parseDateTime("2026-10-10 12:00"), kSecondsPerDay);
}

TEST(DateTime, RejectsImpossibleOrMalformedValues) {
    const char* bad[] = {"", "tomorrow", "2026-13-01 10:00", "2026-00-10 10:00", "2026-02-30 10:00", "2026-04-31 10:00",
                         "2026-10-32 10:00", "2026-10-10 24:00", "2026-10-10 10:60", "2026-10-10 10:00:60", "2026-10-10",
                         "2026-10-10 10:00 extra", "2026-10-10T10:00", "2026/10/10 10:00", "2025-02-29 10:00"};
    for (const char* s : bad) EXPECT_THROW(parseDateTime(s), ValidationException) << s;
}

TEST(DateTime, LeapDayRules) {
    EXPECT_NO_THROW(parseDateTime("2024-02-29 12:00"));
    EXPECT_NO_THROW(parseDateTime("2000-02-29 12:00"));
    EXPECT_THROW(parseDateTime("2100-02-29 12:00"), ValidationException);
    EXPECT_THROW(parseDateTime("2026-02-29 12:00"), ValidationException);
}

TEST(DateTime, MonthEndBoundaries) {
    EXPECT_NO_THROW(parseDateTime("2026-01-31 10:00"));
    EXPECT_NO_THROW(parseDateTime("2026-04-30 10:00"));
    EXPECT_THROW(parseDateTime("2026-04-31 10:00"), ValidationException);
}

TEST(Enums, RoleAndStatusStringRoundTrip) {
    for (Role r : {Role::Student, Role::Lecturer, Role::HeadOfDepartment, Role::Administrator})
        EXPECT_EQ(roleFromString(toString(r)), r);
    for (SubmissionStatus s : {SubmissionStatus::Submitted, SubmissionStatus::Late, SubmissionStatus::Marked,
                               SubmissionStatus::Moderated, SubmissionStatus::Released})
        EXPECT_EQ(statusFromString(toString(s)), s);
}

TEST(Enums, UnknownTextIsRejected) {
    EXPECT_THROW(roleFromString("TEACHER"), ValidationException);
    EXPECT_THROW(statusFromString("DELETED"), ValidationException);
}

TEST(Clock, FixedClockAdvances) {
    FixedClock c(T("2026-10-01 09:00"));
    c.advanceDays(2);
    EXPECT_EQ(formatDateTime(c.now()), std::string("2026-10-03 09:00:00"));
    c.set(T("2026-11-11 11:11"));
    EXPECT_EQ(formatDateTime(c.now()), std::string("2026-11-11 11:11:00"));
}

TEST(Clock, SystemClockReturnsPlausibleTime) {
    SystemClock c;
    EXPECT_GT(c.now(), T("2026-01-01 00:00"));
}

// ------------------------------------------------------------------ LatePenaltyPolicy (BVA)

TEST(TenPercentPolicy, DaysLateBoundaryValues) {
    TenPercentPerDayPolicy p;
    struct Row { int days; int expected; };
    const Row rows[] = {{-5, 80}, {-1, 80}, {0, 80},      // on time: no penalty
                        {1, 70}, {2, 60}, {3, 50},         // 10 marks per day for max 100
                        {4, 0}, {5, 0}, {100, 0}};         // beyond three days: zero
    for (const Row& r : rows) EXPECT_EQ(p.applyPenalty(80, 100, r.days), r.expected) << "days=" << r.days;
}

TEST(TenPercentPolicy, PenaltyNeverGoesBelowZero) {
    TenPercentPerDayPolicy p;
    EXPECT_EQ(p.applyPenalty(20, 100, 3), 0);
    EXPECT_EQ(p.applyPenalty(0, 100, 1), 0);
    EXPECT_EQ(p.applyPenalty(5, 100, 1), 0);
}

TEST(TenPercentPolicy, FullMarksOnTimeAreKept) {
    TenPercentPerDayPolicy p;
    EXPECT_EQ(p.applyPenalty(100, 100, 0), 100);
    EXPECT_EQ(p.applyPenalty(100, 100, 1), 90);
}

TEST(TenPercentPolicy, ScalesWithMaximumMarkNotWithRawMark) {
    TenPercentPerDayPolicy p;
    EXPECT_EQ(p.applyPenalty(40, 50, 1), 35);     // 10% of 50 = 5
    EXPECT_EQ(p.applyPenalty(40, 50, 2), 30);
    EXPECT_EQ(p.applyPenalty(400, 1000, 1), 300); // 10% of 1000 = 100
}

TEST(TenPercentPolicy, FractionalDeductionsRoundToNearestWholeMark) {
    TenPercentPerDayPolicy p;
    EXPECT_EQ(p.applyPenalty(15, 15, 1), 13);     // 1.5 -> 2
    EXPECT_EQ(p.applyPenalty(7, 7, 3), 5);        // 2.1 -> 2
}

TEST(TenPercentPolicy, CustomRateAndLimitAreHonoured) {
    TenPercentPerDayPolicy p(0.25, 2);
    EXPECT_EQ(p.applyPenalty(100, 100, 1), 75);
    EXPECT_EQ(p.applyPenalty(100, 100, 2), 50);
    EXPECT_EQ(p.applyPenalty(100, 100, 3), 0);
}

TEST(TenPercentPolicy, DescribeMentionsTheRule) {
    TenPercentPerDayPolicy p;
    EXPECT_NE(p.describe().find("10%"), std::string::npos);
    EXPECT_NE(p.describe().find("3 days"), std::string::npos);
}

TEST(NoPenaltyPolicy, NeverChangesTheMark) {
    NoPenaltyPolicy p;
    for (int days : {0, 1, 3, 4, 50}) EXPECT_EQ(p.applyPenalty(73, 100, days), 73);
    EXPECT_FALSE(p.describe().empty());
}

// ------------------------------------------------------------------ Deadlines (BVA)

TEST(Deadlines, DaysLateBoundaryValues) {
    const DateTime d = T("2026-10-10 12:00");
    struct Row { DateTime when; int expected; };
    const Row rows[] = {{d - 1000000, 0}, {d - 1, 0}, {d, 0},                     // on time, including exactly at the deadline
                        {d + 1, 1}, {d + kSecondsPerDay, 1},                       // 1 second .. exactly 1 day late = 1 day
                        {d + kSecondsPerDay + 1, 2}, {d + 2 * kSecondsPerDay, 2},
                        {d + 2 * kSecondsPerDay + 1, 3}, {d + 3 * kSecondsPerDay, 3},
                        {d + 3 * kSecondsPerDay + 1, 4}};
    for (const Row& r : rows) EXPECT_EQ(daysLate(r.when, d), r.expected) << (r.when - d);
}

TEST(Deadlines, EffectiveDeadlineUsesTheLaterOfDeadlineAndExtension) {
    Assignment a(1, 1, "A", "", T("2026-10-10 12:00"), 100);
    EXPECT_EQ(effectiveDeadline(a, std::nullopt), T("2026-10-10 12:00"));
    Extension later(1, 1, 1, T("2026-10-13 12:00"), "ill", 1, T("2026-10-01 09:00"));
    EXPECT_EQ(effectiveDeadline(a, later), T("2026-10-13 12:00"));
    Extension earlier(1, 1, 1, T("2026-10-05 12:00"), "x", 1, T("2026-10-01 09:00"));
    EXPECT_EQ(effectiveDeadline(a, earlier), T("2026-10-10 12:00"));
}

TEST(Entities, AssignmentIsOpenUntilAndIncludingTheDeadline) {
    Assignment a(1, 1, "A", "", T("2026-10-10 12:00"), 100);
    EXPECT_TRUE(a.isOpenAt(T("2026-10-10 11:59:59")));
    EXPECT_TRUE(a.isOpenAt(T("2026-10-10 12:00")));
    EXPECT_FALSE(a.isOpenAt(T("2026-10-10 12:00:01")));
}

// ------------------------------------------------------------------ Submission state machine

namespace {
Submission inStatus(SubmissionStatus s) {
    const bool marked = s == SubmissionStatus::Marked || s == SubmissionStatus::Moderated || s == SubmissionStatus::Released;
    return Submission(1, 1, 1, "work", T("2026-10-05 10:00"), 0, s, marked ? std::optional<int>(80) : std::nullopt,
                      marked ? std::optional<int>(80) : std::nullopt, marked ? "ok" : "");
}
const SubmissionStatus kAll[] = {SubmissionStatus::Submitted, SubmissionStatus::Late, SubmissionStatus::Marked,
                                 SubmissionStatus::Moderated, SubmissionStatus::Released};
}  // namespace

TEST(SubmissionLifecycle, NewSubmissionStartsSubmittedOrLate) {
    EXPECT_EQ(Submission::create(1, 1, "w", T("2026-10-05 10:00"), false).getStatus(), SubmissionStatus::Submitted);
    EXPECT_EQ(Submission::create(1, 1, "w", T("2026-10-12 10:00"), true).getStatus(), SubmissionStatus::Late);
    EXPECT_FALSE(Submission::create(1, 1, "w", 0, false).getFinalMark().has_value());
}

TEST(SubmissionLifecycle, ResubmitAllowedWhileSubmittedOrLateAndAlwaysEndsSubmitted) {
    for (SubmissionStatus s : kAll) {
        Submission sub = inStatus(s);
        if (s == SubmissionStatus::Submitted || s == SubmissionStatus::Late) {
            sub.resubmit("v2", T("2026-10-06 10:00"));            // Late -> Submitted is the DEF-002 fix
            EXPECT_EQ(sub.getContent(), std::string("v2"));
            EXPECT_EQ(sub.getResubmitCount(), 1);
            EXPECT_EQ(sub.getSubmittedAt(), T("2026-10-06 10:00"));
            EXPECT_EQ(sub.getStatus(), SubmissionStatus::Submitted);
        } else {
            EXPECT_THROW(sub.resubmit("v2", 0), IllegalStateException) << toString(s);
        }
    }
}

TEST(SubmissionLifecycle, ApplyMarkAllowedUntilModerated) {
    for (SubmissionStatus s : kAll) {
        Submission sub = inStatus(s);
        const bool allowed = s == SubmissionStatus::Submitted || s == SubmissionStatus::Late || s == SubmissionStatus::Marked;
        if (allowed) {
            sub.applyMark(90, 80, "feedback");
            EXPECT_EQ(sub.getStatus(), SubmissionStatus::Marked);
            EXPECT_EQ(sub.getRawMark(), 90);
            EXPECT_EQ(sub.getFinalMark(), 80);
            EXPECT_EQ(sub.getFeedback(), std::string("feedback"));
        } else {
            EXPECT_THROW(sub.applyMark(90, 80, "f"), IllegalStateException) << toString(s);
        }
    }
}

TEST(SubmissionLifecycle, AdjustFinalMarkOnlyWhileMarkedAndKeepsRawMark) {
    for (SubmissionStatus s : kAll) {
        Submission sub = inStatus(s);
        if (s == SubmissionStatus::Marked) {
            sub.adjustFinalMark(65);
            EXPECT_EQ(sub.getFinalMark(), 65);
            EXPECT_EQ(sub.getRawMark(), 80);
            EXPECT_EQ(sub.getStatus(), SubmissionStatus::Marked);
        } else {
            EXPECT_THROW(sub.adjustFinalMark(65), IllegalStateException) << toString(s);
        }
    }
}

TEST(SubmissionLifecycle, ApproveOnlyFromMarked) {
    for (SubmissionStatus s : kAll) {
        Submission sub = inStatus(s);
        if (s == SubmissionStatus::Marked) { sub.approve(); EXPECT_EQ(sub.getStatus(), SubmissionStatus::Moderated); }
        else EXPECT_THROW(sub.approve(), IllegalStateException) << toString(s);
    }
}

TEST(SubmissionLifecycle, ReleaseOnlyFromModerated) {
    for (SubmissionStatus s : kAll) {
        Submission sub = inStatus(s);
        if (s == SubmissionStatus::Moderated) { sub.release(); EXPECT_EQ(sub.getStatus(), SubmissionStatus::Released); }
        else EXPECT_THROW(sub.release(), IllegalStateException) << toString(s);
    }
}

TEST(SubmissionLifecycle, CannotSkipModeration) {
    Submission sub = Submission::create(1, 1, "w", T("2026-10-05 10:00"), false);
    sub.applyMark(70, 70, "ok");
    EXPECT_THROW(sub.release(), IllegalStateException);      // Marked -> Released is not allowed
    sub.approve();
    sub.release();
    EXPECT_EQ(sub.getStatus(), SubmissionStatus::Released);
}

// ------------------------------------------------------------------ Users and factory

TEST(Permissions, EachRoleHasExactlyItsOwnPermissions) {
    const Permission all[] = {Permission::SubmitWork, Permission::ViewReleasedMarks, Permission::CreateAssignment,
                              Permission::GrantExtension, Permission::MarkSubmission, Permission::ModerateMarks,
                              Permission::ReleaseMarks, Permission::ManageUsers, Permission::ManageCourses};
    Student s(1, "s", "h", "x", "S", true, "N1");
    Lecturer l(2, "l", "h", "x", "L", true, "T1", 1);
    HeadOfDepartment h(3, "h", "h", "x", "H", true, "T2", 1);
    Administrator a(4, "a", "h", "x", "A", true);
    // rows: student, lecturer, hod, admin; columns follow `all`
    const bool expected[4][9] = {{1, 1, 0, 0, 0, 0, 0, 0, 0},
                                 {0, 0, 1, 1, 1, 0, 0, 0, 0},
                                 {0, 0, 0, 0, 0, 1, 1, 0, 0},
                                 {0, 0, 0, 0, 0, 0, 0, 1, 1}};
    const User* users[4] = {&s, &l, &h, &a};
    for (int u = 0; u < 4; ++u)
        for (int p = 0; p < 9; ++p)
            EXPECT_EQ(users[u]->canPerform(all[p]), expected[u][p]) << "role " << u << " permission " << p;
}

TEST(Users, RolesAndAccessors) {
    Student s(7, "stu", "hash", "salt", "Stu Dent", true, "N7");
    EXPECT_EQ(s.getRole(), Role::Student);
    EXPECT_EQ(s.getId(), 7);
    EXPECT_EQ(s.getUsername(), std::string("stu"));
    EXPECT_EQ(s.getReferenceNumber(), std::string("N7"));
    EXPECT_FALSE(s.getDepartmentId().has_value());
    s.setActive(false);
    EXPECT_FALSE(s.isActive());
    s.setId(9);
    EXPECT_EQ(s.getId(), 9);
    Lecturer l(1, "l", "h", "s", "L", true, "T1", 4);
    EXPECT_EQ(l.getDepartmentId(), 4);
    EXPECT_EQ(l.getRole(), Role::Lecturer);
    HeadOfDepartment h(1, "h", "h", "s", "H", true, "T2", 5);
    EXPECT_EQ(h.getDepartmentId(), 5);
    EXPECT_EQ(h.getRole(), Role::HeadOfDepartment);
    Administrator a(1, "a", "h", "s", "A", true);
    EXPECT_EQ(a.getRole(), Role::Administrator);
    EXPECT_EQ(a.getReferenceNumber(), std::string(""));
}

TEST(UserFactory, CreatesTheRightConcreteType) {
    UserDetails d;
    d.id = 5; d.username = "u"; d.passwordHash = "h"; d.salt = "s"; d.fullName = "F"; d.referenceNumber = "R1"; d.departmentId = 3;
    std::unique_ptr<User> s = UserFactory::create(Role::Student, d);
    EXPECT_TRUE(dynamic_cast<Student*>(s.get()) != nullptr);
    std::unique_ptr<User> l = UserFactory::create(Role::Lecturer, d);
    EXPECT_TRUE(dynamic_cast<Lecturer*>(l.get()) != nullptr);
    EXPECT_EQ(l->getDepartmentId(), 3);
    std::unique_ptr<User> h = UserFactory::create(Role::HeadOfDepartment, d);
    EXPECT_TRUE(dynamic_cast<HeadOfDepartment*>(h.get()) != nullptr);
    std::unique_ptr<User> a = UserFactory::create(Role::Administrator, d);
    EXPECT_TRUE(dynamic_cast<Administrator*>(a.get()) != nullptr);
    EXPECT_EQ(a->getId(), 5);
    EXPECT_EQ(a->getFullName(), std::string("F"));
}

TEST(UserFactory, RejectsIncompleteDetails) {
    UserDetails noDept;
    noDept.username = "u"; noDept.referenceNumber = "R";
    EXPECT_THROW(UserFactory::create(Role::Lecturer, noDept), ValidationException);
    EXPECT_THROW(UserFactory::create(Role::HeadOfDepartment, noDept), ValidationException);
    UserDetails noNumber;
    noNumber.username = "u"; noNumber.departmentId = 1;
    EXPECT_THROW(UserFactory::create(Role::Student, noNumber), ValidationException);
    EXPECT_THROW(UserFactory::create(Role::Lecturer, noNumber), ValidationException);
    EXPECT_NO_THROW(UserFactory::create(Role::Administrator, noNumber));
}
