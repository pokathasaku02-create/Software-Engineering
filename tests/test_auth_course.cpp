// UNIT / INTEGRATION tests: password hashing, authentication, course and assignment services.
#include <set>

#include "test_support.h"

using namespace lms;
using namespace lms::test;

// ------------------------------------------------------------------ PasswordHasher

TEST(PasswordHasher, SaltsAreRandomAndCorrectLength) {
    PasswordHasher h(true);
    std::set<std::string> seen;
    for (int i = 0; i < 20; ++i) {
        std::string s = h.generateSalt();
        EXPECT_EQ(s.size(), 32u);
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), 20u);
}

TEST(PasswordHasher, HashIsDeterministicForSamePasswordAndSalt) {
    PasswordHasher h(true);
    const std::string salt = h.generateSalt();
    EXPECT_EQ(h.hash("correct horse", salt), h.hash("correct horse", salt));
    EXPECT_EQ(h.hash("correct horse", salt).size(), 64u);
}

TEST(PasswordHasher, DifferentSaltOrPasswordGivesDifferentHash) {
    PasswordHasher h(true);
    const std::string s1 = h.generateSalt(), s2 = h.generateSalt();
    EXPECT_NE(h.hash("password1", s1), h.hash("password1", s2));
    EXPECT_NE(h.hash("password1", s1), h.hash("password2", s1));
}

TEST(PasswordHasher, VerifyAcceptsOnlyTheRightPassword) {
    PasswordHasher h(true);
    const std::string salt = h.generateSalt();
    const std::string hash = h.hash("S3cret pass phrase!", salt);
    EXPECT_TRUE(h.verify("S3cret pass phrase!", salt, hash));
    EXPECT_FALSE(h.verify("S3cret pass phrase", salt, hash));
    EXPECT_FALSE(h.verify("", salt, hash));
    EXPECT_FALSE(h.verify("s3cret pass phrase!", salt, hash));
}

TEST(PasswordHasher, CorruptStoredValuesNeverVerify) {
    PasswordHasher h(true);
    const std::string salt = h.generateSalt();
    const std::string hash = h.hash("password1", salt);
    EXPECT_FALSE(h.verify("password1", salt, "not-hex"));
    EXPECT_FALSE(h.verify("password1", "zz", hash));
    EXPECT_FALSE(h.verify("password1", salt, hash.substr(0, 10)));
    EXPECT_THROW(h.hash("password1", "not-a-salt"), LmsException);
}

TEST(PasswordHasher, HandlesSpacesAndUnicode) {
    PasswordHasher h(true);
    const std::string salt = h.generateSalt();
    const std::string pw = "pass word \xC3\xA9\xE2\x82\xAC";
    EXPECT_TRUE(h.verify(pw, salt, h.hash(pw, salt)));
}

// ------------------------------------------------------------------ AuthService

class AuthTest : public LmsTest {};

TEST_F(AuthTest, LoginSucceedsWithCorrectCredentials) {
    const User* u = w.auth.login("student1", "password1");
    ASSERT_TRUE(u != nullptr);
    EXPECT_EQ(u->getRole(), Role::Student);
    EXPECT_TRUE(w.auth.currentUser() == u);
}

TEST_F(AuthTest, LoginFailuresShareOneGenericMessage) {
    std::string m1, m2, m3;
    try { w.auth.login("student1", "wrongpass"); } catch (const AuthenticationException& e) { m1 = e.what(); }
    try { w.auth.login("nobody", "password1"); } catch (const AuthenticationException& e) { m2 = e.what(); }
    std::unique_ptr<User> off = w.userRepo.findById(w.student2->getId());
    off->setActive(false);
    w.userRepo.update(*off);
    try { w.auth.login("student2", "password1"); } catch (const AuthenticationException& e) { m3 = e.what(); }
    EXPECT_FALSE(m1.empty());
    EXPECT_EQ(m1, m2);
    EXPECT_EQ(m1, m3);
    EXPECT_TRUE(w.auth.currentUser() == nullptr);
}

TEST_F(AuthTest, LogoutClearsTheSession) {
    w.auth.login("student1", "password1");
    w.auth.logout();
    EXPECT_TRUE(w.auth.currentUser() == nullptr);
}

TEST_F(AuthTest, LoginIsCaseSensitiveForUsernameAndPassword) {
    EXPECT_THROW(w.auth.login("STUDENT1", "password1"), AuthenticationException);
    EXPECT_THROW(w.auth.login("student1", "PASSWORD1"), AuthenticationException);
}

TEST_F(AuthTest, SqlInjectionInLoginFails) {
    EXPECT_THROW(w.auth.login("' OR '1'='1", "' OR '1'='1"), AuthenticationException);
    EXPECT_THROW(w.auth.login("student1' --", "x"), AuthenticationException);
}

TEST_F(AuthTest, PlainTextPasswordIsNeverStored) {
    std::unique_ptr<User> u = w.userRepo.findByUsername("student1");
    EXPECT_NE(u->getPasswordHash(), std::string("password1"));
    EXPECT_EQ(u->getPasswordHash().size(), 64u);
    EXPECT_EQ(u->getPasswordHash().find("password1"), std::string::npos);
    // scan the whole users table for the clear-text password
    Statement s = w.db.prepare("SELECT COUNT(*) FROM users WHERE password_hash LIKE ? OR salt LIKE ? OR full_name LIKE ?");
    s.bind(1, std::string("%password1%")).bind(2, std::string("%password1%")).bind(3, std::string("%password1%"));
    ASSERT_TRUE(s.step());
    EXPECT_EQ(s.columnInt(0), 0);
}

TEST_F(AuthTest, SamePasswordGetsDifferentHashesBecauseOfSalt) {
    EXPECT_NE(w.student1->getPasswordHash(), w.student2->getPasswordHash());
    EXPECT_NE(w.student1->getSalt(), w.student2->getSalt());
}

TEST_F(AuthTest, OnlyAdministratorsCreateAccounts) {
    UserDetails d;
    d.username = "newbie"; d.fullName = "New"; d.referenceNumber = "N5";
    EXPECT_THROW(w.auth.createUser(*w.student1, Role::Student, d, "password1"), AccessDeniedException);
    EXPECT_THROW(w.auth.createUser(*w.lecturer, Role::Student, d, "password1"), AccessDeniedException);
    EXPECT_THROW(w.auth.createUser(*w.hod, Role::Student, d, "password1"), AccessDeniedException);
    EXPECT_NO_THROW(w.auth.createUser(*w.admin, Role::Student, d, "password1"));
}

TEST_F(AuthTest, DisabledAdministratorCannotAct) {
    std::unique_ptr<User> a = w.userRepo.findById(w.admin->getId());
    a->setActive(false);
    UserDetails d;
    d.username = "newbie"; d.fullName = "New"; d.referenceNumber = "N5";
    EXPECT_THROW(w.auth.createUser(*a, Role::Student, d, "password1"), AccessDeniedException);
}

TEST_F(AuthTest, DuplicateUsernameAndMissingNameAreRejected) {
    UserDetails d;
    d.username = "student1"; d.fullName = "Again"; d.referenceNumber = "N5";
    EXPECT_THROW(w.auth.createUser(*w.admin, Role::Student, d, "password1"), ValidationException);
    d.username = "fresh"; d.fullName = "";
    EXPECT_THROW(w.auth.createUser(*w.admin, Role::Student, d, "password1"), ValidationException);
}

// Regression test for DEF-003: a clear message instead of a raw database error.
TEST_F(AuthTest, SecondHeadOfDepartmentForTheSameDepartmentGetsAClearMessage) {
    UserDetails d;
    d.username = "hod_second"; d.fullName = "Second"; d.referenceNumber = "T7"; d.departmentId = w.dept;
    std::string message;
    try { w.auth.createUser(*w.admin, Role::HeadOfDepartment, d, "password1"); } catch (const ValidationException& e) { message = e.what(); }
    EXPECT_NE(message.find("already has a head of department"), std::string::npos) << message;
    const int arts = w.courseSvc.createDepartment(*w.admin, "Arts");
    d.departmentId = arts;
    EXPECT_NO_THROW(w.auth.createUser(*w.admin, Role::HeadOfDepartment, d, "password1"));
}

TEST_F(AuthTest, LecturerWithoutDepartmentIsRejected) {
    UserDetails d;
    d.username = "nodept"; d.fullName = "No Dept"; d.referenceNumber = "T9";
    EXPECT_THROW(w.auth.createUser(*w.admin, Role::Lecturer, d, "password1"), ValidationException);
}

TEST_F(AuthTest, PasswordLengthBoundaries) {
    EXPECT_THROW(AuthService::validatePassword(std::string(7, 'a')), ValidationException);
    EXPECT_NO_THROW(AuthService::validatePassword(std::string(8, 'a')));
    EXPECT_NO_THROW(AuthService::validatePassword(std::string(128, 'a')));
    EXPECT_THROW(AuthService::validatePassword(std::string(129, 'a')), ValidationException);
    EXPECT_THROW(AuthService::validatePassword(""), ValidationException);
}

TEST_F(AuthTest, UsernameBoundariesAndCharacters) {
    EXPECT_THROW(AuthService::validateUsername("ab"), ValidationException);
    EXPECT_NO_THROW(AuthService::validateUsername("abc"));
    EXPECT_NO_THROW(AuthService::validateUsername(std::string(32, 'x')));
    EXPECT_THROW(AuthService::validateUsername(std::string(33, 'x')), ValidationException);
    EXPECT_NO_THROW(AuthService::validateUsername("a.b-c_d9"));
    for (const char* bad : {"has space", "semi;colon", "quote'", "percent%", "tab\tx", "uni\xC3\xA9"})
        EXPECT_THROW(AuthService::validateUsername(bad), ValidationException) << bad;
}

TEST_F(AuthTest, ListUsersIsAdministratorOnly) {
    EXPECT_EQ(w.auth.listUsers(*w.admin).size(), 6u);
    EXPECT_THROW(w.auth.listUsers(*w.student1), AccessDeniedException);
}

TEST(AuthSetup, InitialAdministratorOnlyWhenNoUsersExist) {
    World world;
    EXPECT_FALSE(world.auth.hasAnyUsers());
    EXPECT_THROW(world.auth.createInitialAdministrator("ab", "Admin", "adminpass1"), ValidationException);
    EXPECT_THROW(world.auth.createInitialAdministrator("admin", "Admin", "short"), ValidationException);
    EXPECT_NO_THROW(world.auth.createInitialAdministrator("admin", "Admin", "adminpass1"));
    EXPECT_TRUE(world.auth.hasAnyUsers());
    EXPECT_THROW(world.auth.createInitialAdministrator("admin2", "Admin", "adminpass1"), AccessDeniedException);
}

// ------------------------------------------------------------------ CourseService

class CourseServiceTest : public LmsTest {};

TEST_F(CourseServiceTest, DepartmentRules) {
    EXPECT_THROW(w.courseSvc.createDepartment(*w.lecturer, "Arts"), AccessDeniedException);
    EXPECT_THROW(w.courseSvc.createDepartment(*w.admin, ""), ValidationException);
    EXPECT_THROW(w.courseSvc.createDepartment(*w.admin, "Science"), ValidationException);
    EXPECT_NO_THROW(w.courseSvc.createDepartment(*w.admin, std::string(100, 'd')));
    EXPECT_THROW(w.courseSvc.createDepartment(*w.admin, std::string(101, 'e')), ValidationException);
    EXPECT_EQ(w.courseSvc.departments().size(), 2u);
}

TEST_F(CourseServiceTest, CourseRules) {
    CourseData c;
    c.code = "MA101"; c.title = "Maths"; c.departmentId = w.dept;
    EXPECT_THROW(w.courseSvc.createCourse(*w.student1, c), AccessDeniedException);
    EXPECT_NO_THROW(w.courseSvc.createCourse(*w.admin, c));
    EXPECT_THROW(w.courseSvc.createCourse(*w.admin, c), ValidationException);          // duplicate code
    c.code = "X1"; c.departmentId = 9999;
    EXPECT_THROW(w.courseSvc.createCourse(*w.admin, c), NotFoundException);
    c.departmentId = w.dept; c.code = std::string(20, 'c');
    EXPECT_NO_THROW(w.courseSvc.createCourse(*w.admin, c));
    c.code = std::string(21, 'c');
    EXPECT_THROW(w.courseSvc.createCourse(*w.admin, c), ValidationException);
    c.code = ""; EXPECT_THROW(w.courseSvc.createCourse(*w.admin, c), ValidationException);
    c.code = "OK"; c.title = "";
    EXPECT_THROW(w.courseSvc.createCourse(*w.admin, c), ValidationException);
}

TEST_F(CourseServiceTest, AssignLecturerRules) {
    const int other = w.courseSvc.createDepartment(*w.admin, "Arts");
    std::unique_ptr<User> artsLecturer = w.addUser(Role::Lecturer, "arts_lec", other);
    EXPECT_THROW(w.courseSvc.assignLecturer(*w.student1, w.course, w.lecturer->getId()), AccessDeniedException);
    EXPECT_THROW(w.courseSvc.assignLecturer(*w.admin, 9999, w.lecturer->getId()), NotFoundException);
    EXPECT_THROW(w.courseSvc.assignLecturer(*w.admin, w.course, w.student1->getId()), ValidationException);   // not a lecturer
    EXPECT_THROW(w.courseSvc.assignLecturer(*w.admin, w.course, 99999), ValidationException);
    EXPECT_THROW(w.courseSvc.assignLecturer(*w.admin, w.course, artsLecturer->getId()), ValidationException); // other department
    std::unique_ptr<User> second = w.addUser(Role::Lecturer, "sci_lec2", w.dept);
    EXPECT_NO_THROW(w.courseSvc.assignLecturer(*w.admin, w.course, second->getId()));
    EXPECT_EQ(w.courseRepo.findById(w.course)->getLecturerId(), second->getId());
}

TEST_F(CourseServiceTest, DisabledLecturerCannotBeAssigned) {
    std::unique_ptr<User> second = w.addUser(Role::Lecturer, "sci_lec3", w.dept);
    second->setActive(false);
    w.userRepo.update(*second);
    EXPECT_THROW(w.courseSvc.assignLecturer(*w.admin, w.course, second->getId()), ValidationException);
}

TEST_F(CourseServiceTest, EnrolmentRules) {
    std::unique_ptr<User> s3 = w.addUser(Role::Student, "student3");
    EXPECT_THROW(w.courseSvc.enrolStudent(*w.lecturer, w.course, s3->getId()), AccessDeniedException);
    EXPECT_THROW(w.courseSvc.enrolStudent(*w.admin, 9999, s3->getId()), NotFoundException);
    EXPECT_THROW(w.courseSvc.enrolStudent(*w.admin, w.course, w.lecturer->getId()), ValidationException);   // not a student
    EXPECT_THROW(w.courseSvc.enrolStudent(*w.admin, w.course, 99999), ValidationException);
    EXPECT_THROW(w.courseSvc.enrolStudent(*w.admin, w.course, w.student1->getId()), ValidationException);   // already enrolled
    EXPECT_NO_THROW(w.courseSvc.enrolStudent(*w.admin, w.course, s3->getId()));
    EXPECT_EQ(w.courseSvc.coursesFor(*s3).size(), 1u);
}

TEST_F(CourseServiceTest, CoursesForEachRole) {
    EXPECT_EQ(w.courseSvc.coursesFor(*w.student1).size(), 1u);
    EXPECT_EQ(w.courseSvc.coursesFor(*w.lecturer).size(), 1u);
    EXPECT_EQ(w.courseSvc.coursesFor(*w.hod).size(), 1u);
    EXPECT_EQ(w.courseSvc.coursesFor(*w.admin).size(), 1u);
    std::unique_ptr<User> lonely = w.addUser(Role::Lecturer, "lonely_lec", w.dept);     // teaches nothing
    EXPECT_EQ(w.courseSvc.coursesFor(*lonely).size(), 0u);
    const int arts = w.courseSvc.createDepartment(*w.admin, "Arts");
    std::unique_ptr<User> artsHod = w.addUser(Role::HeadOfDepartment, "arts_hod", arts);
    EXPECT_EQ(w.courseSvc.coursesFor(*artsHod).size(), 0u);
}

// ------------------------------------------------------------------ AssignmentService

class AssignmentServiceTest : public LmsTest {
protected:
    AssignmentData data(const char* deadline = "2026-10-20 12:00") {
        AssignmentData a;
        a.courseId = w.course; a.title = "New one"; a.description = "d"; a.deadline = T(deadline); a.maxMark = 50;
        return a;
    }
    ExtensionData ext(const char* newDeadline = "2026-10-13 12:00") {
        ExtensionData e;
        e.assignmentId = w.assignmentId; e.studentId = w.student1->getId(); e.newDeadline = T(newDeadline); e.reason = "illness";
        return e;
    }
};

TEST_F(AssignmentServiceTest, OnlyTheCourseLecturerCreatesAssignments) {
    EXPECT_NO_THROW(w.assignmentSvc.createAssignment(*w.lecturer, data()));
    EXPECT_THROW(w.assignmentSvc.createAssignment(*w.student1, data()), AccessDeniedException);
    EXPECT_THROW(w.assignmentSvc.createAssignment(*w.hod, data()), AccessDeniedException);
    std::unique_ptr<User> other = w.addUser(Role::Lecturer, "other_lec", w.dept);
    EXPECT_THROW(w.assignmentSvc.createAssignment(*other, data()), AccessDeniedException);   // does not teach the course
    AssignmentData bad = data(); bad.courseId = 9999;
    EXPECT_THROW(w.assignmentSvc.createAssignment(*w.lecturer, bad), NotFoundException);
}

TEST_F(AssignmentServiceTest, MaxMarkBoundaries) {
    for (int m : {0, -1, 1001}) { AssignmentData a = data(); a.maxMark = m; EXPECT_THROW(w.assignmentSvc.createAssignment(*w.lecturer, a), ValidationException) << m; }
    for (int m : {1, 1000}) { AssignmentData a = data(); a.maxMark = m; EXPECT_NO_THROW(w.assignmentSvc.createAssignment(*w.lecturer, a)) << m; }
}

TEST_F(AssignmentServiceTest, TitleAndDescriptionBoundaries) {
    AssignmentData a = data();
    a.title = ""; EXPECT_THROW(w.assignmentSvc.createAssignment(*w.lecturer, a), ValidationException);
    a.title = std::string(100, 't'); EXPECT_NO_THROW(w.assignmentSvc.createAssignment(*w.lecturer, a));
    a.title = std::string(101, 't'); EXPECT_THROW(w.assignmentSvc.createAssignment(*w.lecturer, a), ValidationException);
    a = data(); a.description = std::string(2000, 'd'); EXPECT_NO_THROW(w.assignmentSvc.createAssignment(*w.lecturer, a));
    a.description = std::string(2001, 'd'); EXPECT_THROW(w.assignmentSvc.createAssignment(*w.lecturer, a), ValidationException);
}

TEST_F(AssignmentServiceTest, DeadlineMustBeInTheFuture) {
    at("2026-10-01 09:00");
    EXPECT_THROW(w.assignmentSvc.createAssignment(*w.lecturer, data("2026-10-01 08:59")), ValidationException);
    EXPECT_THROW(w.assignmentSvc.createAssignment(*w.lecturer, data("2026-10-01 09:00")), ValidationException);   // equal to now
    EXPECT_NO_THROW(w.assignmentSvc.createAssignment(*w.lecturer, data("2026-10-01 09:01")));
}

TEST_F(AssignmentServiceTest, ExtensionRules) {
    EXPECT_NO_THROW(w.assignmentSvc.grantExtension(*w.lecturer, ext()));
    EXPECT_THROW(w.assignmentSvc.grantExtension(*w.student1, ext()), AccessDeniedException);
    EXPECT_THROW(w.assignmentSvc.grantExtension(*w.hod, ext()), AccessDeniedException);
    std::unique_ptr<User> other = w.addUser(Role::Lecturer, "other_lec", w.dept);
    EXPECT_THROW(w.assignmentSvc.grantExtension(*other, ext()), AccessDeniedException);
    ExtensionData e = ext(); e.assignmentId = 9999;
    EXPECT_THROW(w.assignmentSvc.grantExtension(*w.lecturer, e), NotFoundException);
}

TEST_F(AssignmentServiceTest, ExtensionDeadlineBoundaries) {
    EXPECT_THROW(w.assignmentSvc.grantExtension(*w.lecturer, ext("2026-10-09 12:00")), ValidationException);
    EXPECT_THROW(w.assignmentSvc.grantExtension(*w.lecturer, ext("2026-10-10 12:00")), ValidationException);   // equal: not an extension
    EXPECT_NO_THROW(w.assignmentSvc.grantExtension(*w.lecturer, ext("2026-10-10 12:00:01")));
}

TEST_F(AssignmentServiceTest, ExtensionNeedsEnrolledStudentAndReason) {
    std::unique_ptr<User> stranger = w.addUser(Role::Student, "stranger");
    ExtensionData e = ext(); e.studentId = stranger->getId();
    EXPECT_THROW(w.assignmentSvc.grantExtension(*w.lecturer, e), ValidationException);
    e = ext(); e.reason = "";
    EXPECT_THROW(w.assignmentSvc.grantExtension(*w.lecturer, e), ValidationException);
    e.reason = std::string(500, 'r'); EXPECT_NO_THROW(w.assignmentSvc.grantExtension(*w.lecturer, e));
    e.reason = std::string(501, 'r'); EXPECT_THROW(w.assignmentSvc.grantExtension(*w.lecturer, e), ValidationException);
}

TEST_F(AssignmentServiceTest, SecondExtensionReplacesTheFirst) {
    w.assignmentSvc.grantExtension(*w.lecturer, ext("2026-10-12 12:00"));
    w.assignmentSvc.grantExtension(*w.lecturer, ext("2026-10-15 12:00"));
    EXPECT_EQ(w.assignmentRepo.findExtension(w.student1->getId(), w.assignmentId)->getNewDeadline(), T("2026-10-15 12:00"));
}

TEST_F(AssignmentServiceTest, StudentsSeeOnlyAssignmentsOfTheirCourses) {
    EXPECT_EQ(w.assignmentSvc.assignmentsFor(*w.student1).size(), 1u);
    const int other = w.courseSvc.createCourse(*w.admin, CourseData{"PH101", "Physics", w.dept});
    w.courseSvc.assignLecturer(*w.admin, other, w.lecturer->getId());
    AssignmentData a = data(); a.courseId = other;
    w.assignmentSvc.createAssignment(*w.lecturer, a);
    EXPECT_EQ(w.assignmentSvc.assignmentsFor(*w.student1).size(), 1u);    // not enrolled in PH101
    w.courseSvc.enrolStudent(*w.admin, other, w.student1->getId());
    EXPECT_EQ(w.assignmentSvc.assignmentsFor(*w.student1).size(), 2u);
    EXPECT_THROW(w.assignmentSvc.assignmentsFor(*w.lecturer), AccessDeniedException);
}

TEST_F(AssignmentServiceTest, CourseAssignmentListVisibleToLecturerAndOwnHodOnly) {
    EXPECT_EQ(w.assignmentSvc.assignmentsForCourse(*w.lecturer, w.course).size(), 1u);
    EXPECT_EQ(w.assignmentSvc.assignmentsForCourse(*w.hod, w.course).size(), 1u);
    EXPECT_THROW(w.assignmentSvc.assignmentsForCourse(*w.student1, w.course), AccessDeniedException);
    EXPECT_THROW(w.assignmentSvc.assignmentsForCourse(*w.admin, w.course), AccessDeniedException);
    EXPECT_THROW(w.assignmentSvc.assignmentsForCourse(*w.lecturer, 9999), NotFoundException);
    const int arts = w.courseSvc.createDepartment(*w.admin, "Arts");
    std::unique_ptr<User> artsHod = w.addUser(Role::HeadOfDepartment, "arts_hod", arts);
    EXPECT_THROW(w.assignmentSvc.assignmentsForCourse(*artsHod, w.course), AccessDeniedException);
}

TEST_F(AssignmentServiceTest, GetAssignment) {
    EXPECT_EQ(w.assignmentSvc.getAssignment(w.assignmentId).getTitle(), std::string("Assignment 1"));
    EXPECT_THROW(w.assignmentSvc.getAssignment(9999), NotFoundException);
}
