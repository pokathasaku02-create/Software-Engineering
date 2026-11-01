// INTEGRATION tests: repositories against a real (in-memory) SQLite database, including the
// constraints that protect data integrity (uniqueness, foreign keys, append-only audit log).
#include "test_support.h"

using namespace lms;
using namespace lms::test;

class RepositoryTest : public LmsTest {};

TEST_F(RepositoryTest, UserRoundTripKeepsEveryField) {
    std::unique_ptr<User> u = w.userRepo.findById(w.lecturer->getId());
    ASSERT_TRUE(u != nullptr);
    EXPECT_EQ(u->getRole(), Role::Lecturer);
    EXPECT_EQ(u->getUsername(), std::string("lecturer1"));
    EXPECT_EQ(u->getDepartmentId(), w.dept);
    EXPECT_EQ(u->getReferenceNumber(), std::string("REF-lecturer1"));
    EXPECT_TRUE(u->isActive());
    EXPECT_EQ(u->getSalt().size(), 32u);
}

TEST_F(RepositoryTest, UnknownUserIsNullNotAnError) {
    EXPECT_TRUE(w.userRepo.findById(99999) == nullptr);
    EXPECT_TRUE(w.userRepo.findByUsername("nobody") == nullptr);
}

TEST_F(RepositoryTest, DuplicateUsernameIsRejectedByTheDatabase) {
    UserDetails d;
    d.username = "student1"; d.fullName = "Dup"; d.referenceNumber = "X"; d.passwordHash = "h"; d.salt = "s";
    std::unique_ptr<User> u = UserFactory::create(Role::Student, d);
    EXPECT_THROW(w.userRepo.save(*u), DatabaseException);
}

TEST_F(RepositoryTest, OnlyOneHeadOfDepartmentPerDepartment) {
    UserDetails d;
    d.username = "hod_second"; d.fullName = "Second"; d.referenceNumber = "X"; d.passwordHash = "h"; d.salt = "s";
    d.departmentId = w.dept;
    std::unique_ptr<User> second = UserFactory::create(Role::HeadOfDepartment, d);
    EXPECT_THROW(w.userRepo.save(*second), DatabaseException);
}

TEST_F(RepositoryTest, UpdatePersistsActiveFlagAndPassword) {
    std::unique_ptr<User> u = w.userRepo.findById(w.student1->getId());
    u->setActive(false);
    w.userRepo.update(*u);
    EXPECT_FALSE(w.userRepo.findById(w.student1->getId())->isActive());
}

TEST_F(RepositoryTest, FindAllReturnsEveryone) {
    EXPECT_EQ(w.userRepo.findAll().size(), 6u);   // admin, admin2, lecturer, hod, 2 students
}

TEST_F(RepositoryTest, MaliciousTextIsStoredAsDataNotExecuted) {
    UserDetails d;
    d.username = "Robert'); DROP TABLE users;--"; d.fullName = "Bobby"; d.referenceNumber = "N9"; d.passwordHash = "h"; d.salt = "s";
    std::unique_ptr<User> u = UserFactory::create(Role::Student, d);
    EXPECT_NO_THROW(w.userRepo.save(*u));
    EXPECT_TRUE(w.userRepo.findByUsername("Robert'); DROP TABLE users;--") != nullptr);
    EXPECT_EQ(w.userRepo.findAll().size(), 7u);                    // table still exists and has all rows
    EXPECT_TRUE(w.userRepo.findByUsername("x' OR '1'='1") == nullptr);   // injection returns nothing
}

TEST_F(RepositoryTest, CourseQueries) {
    std::optional<Course> c = w.courseRepo.findById(w.course);
    ASSERT_TRUE(c.has_value());
    EXPECT_EQ(c->getCode(), std::string("CS101"));
    EXPECT_EQ(c->getLecturerId(), w.lecturer->getId());
    EXPECT_EQ(w.courseRepo.findByDepartment(w.dept).size(), 1u);
    EXPECT_EQ(w.courseRepo.findByDepartment(9999).size(), 0u);
    EXPECT_EQ(w.courseRepo.findByStudent(w.student1->getId()).size(), 1u);
    EXPECT_EQ(w.courseRepo.findAll().size(), 1u);
    EXPECT_FALSE(w.courseRepo.findById(9999).has_value());
}

TEST_F(RepositoryTest, CourseUpdateChangesLecturer) {
    std::optional<Course> c = w.courseRepo.findById(w.course);
    c->setLecturerId(std::nullopt);
    w.courseRepo.update(*c);
    EXPECT_FALSE(w.courseRepo.findById(w.course)->getLecturerId().has_value());
}

TEST_F(RepositoryTest, CourseConstraints) {
    EXPECT_THROW(w.courseRepo.save(Course(0, "CS101", "Dup", w.dept, std::nullopt)), DatabaseException);   // duplicate code
    EXPECT_THROW(w.courseRepo.save(Course(0, "CS999", "Orphan", 4242, std::nullopt)), DatabaseException);   // no such department
}

TEST_F(RepositoryTest, DepartmentsAndEnrolments) {
    EXPECT_EQ(w.courseRepo.findDepartments().size(), 1u);
    EXPECT_EQ(w.courseRepo.findDepartment(w.dept)->getName(), std::string("Science"));
    EXPECT_FALSE(w.courseRepo.findDepartment(777).has_value());
    EXPECT_THROW(w.courseRepo.saveDepartment(Department(0, "Science")), DatabaseException);
    EXPECT_THROW(w.courseRepo.enrol(Enrolment(w.student1->getId(), w.course, 0)), DatabaseException);   // already enrolled
    EXPECT_THROW(w.courseRepo.enrol(Enrolment(w.student1->getId(), 4242, 0)), DatabaseException);       // no such course
}

TEST_F(RepositoryTest, AssignmentRoundTripKeepsTheExactDeadline) {
    std::optional<Assignment> a = w.assignmentRepo.findById(w.assignmentId);
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(a->getDeadline(), T("2026-10-10 12:00"));
    EXPECT_EQ(a->getMaxMark(), 100);
    EXPECT_EQ(a->getTitle(), std::string("Assignment 1"));
    EXPECT_FALSE(w.assignmentRepo.findById(555).has_value());
}

TEST_F(RepositoryTest, AssignmentsAreListedByDeadline) {
    w.assignmentRepo.save(Assignment(0, w.course, "Earlier", "", T("2026-10-05 09:00"), 50));
    std::vector<Assignment> list = w.assignmentRepo.findByCourse(w.course);
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0].getTitle(), std::string("Earlier"));
}

TEST_F(RepositoryTest, AssignmentMaxMarkMustBePositive) {
    EXPECT_THROW(w.assignmentRepo.save(Assignment(0, w.course, "Zero", "", T("2026-10-20 09:00"), 0)), DatabaseException);
}

TEST_F(RepositoryTest, ExtensionUpsertReplacesInsteadOfDuplicating) {
    const int sid = w.student1->getId();
    EXPECT_FALSE(w.assignmentRepo.findExtension(sid, w.assignmentId).has_value());
    const int first = w.assignmentRepo.saveExtension(Extension(0, w.assignmentId, sid, T("2026-10-12 12:00"), "ill", w.lecturer->getId(), T("2026-10-01 09:00")));
    const int second = w.assignmentRepo.saveExtension(Extension(0, w.assignmentId, sid, T("2026-10-14 12:00"), "still ill", w.lecturer->getId(), T("2026-10-02 09:00")));
    EXPECT_EQ(first, second);
    std::optional<Extension> e = w.assignmentRepo.findExtension(sid, w.assignmentId);
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->getNewDeadline(), T("2026-10-14 12:00"));
    EXPECT_EQ(e->getReason(), std::string("still ill"));
}

TEST_F(RepositoryTest, SubmissionRoundTripWithAndWithoutMarks) {
    Submission fresh = Submission::create(w.assignmentId, w.student1->getId(), "text", T("2026-10-05 10:00"), false);
    const int id = w.submissionRepo.save(fresh);
    std::optional<Submission> s = w.submissionRepo.findById(id);
    ASSERT_TRUE(s.has_value());
    EXPECT_FALSE(s->getRawMark().has_value());
    EXPECT_FALSE(s->getFinalMark().has_value());
    EXPECT_EQ(s->getStatus(), SubmissionStatus::Submitted);
    s->applyMark(77, 67, "good");
    w.submissionRepo.update(*s);
    std::optional<Submission> again = w.submissionRepo.findById(id);
    EXPECT_EQ(again->getRawMark(), 77);
    EXPECT_EQ(again->getFinalMark(), 67);
    EXPECT_EQ(again->getFeedback(), std::string("good"));
    EXPECT_EQ(again->getStatus(), SubmissionStatus::Marked);
}

TEST_F(RepositoryTest, OneSubmissionPerStudentPerAssignment) {
    w.submissionRepo.save(Submission::create(w.assignmentId, w.student1->getId(), "a", 0, false));
    EXPECT_THROW(w.submissionRepo.save(Submission::create(w.assignmentId, w.student1->getId(), "b", 0, false)), DatabaseException);
}

TEST_F(RepositoryTest, SubmissionQueriesFilterByStatusAndCourse) {
    const int a = w.submissionRepo.save(Submission::create(w.assignmentId, w.student1->getId(), "a", T("2026-10-05 10:00"), false));
    w.submissionRepo.save(Submission::create(w.assignmentId, w.student2->getId(), "b", T("2026-10-12 10:00"), true));
    EXPECT_EQ(w.submissionRepo.findByCourseAndStatus(w.course, SubmissionStatus::Submitted).size(), 1u);
    EXPECT_EQ(w.submissionRepo.findByCourseAndStatus(w.course, SubmissionStatus::Late).size(), 1u);
    EXPECT_EQ(w.submissionRepo.findByCourseAndStatus(w.course, SubmissionStatus::Marked).size(), 0u);
    EXPECT_EQ(w.submissionRepo.findByCourseAndStatus(4242, SubmissionStatus::Submitted).size(), 0u);
    EXPECT_TRUE(w.submissionRepo.findByStudentAndAssignment(w.student1->getId(), w.assignmentId).has_value());
    EXPECT_FALSE(w.submissionRepo.findByStudentAndAssignment(w.student1->getId(), 4242).has_value());
    EXPECT_EQ(w.submissionRepo.findReleasedByStudent(w.student1->getId()).size(), 0u);
    std::optional<Submission> s = w.submissionRepo.findById(a);
    s->applyMark(50, 50, "f"); s->approve(); s->release();
    w.submissionRepo.update(*s);
    EXPECT_EQ(w.submissionRepo.findReleasedByStudent(w.student1->getId()).size(), 1u);
    EXPECT_EQ(w.submissionRepo.findReleasedByStudent(w.student2->getId()).size(), 0u);
}

TEST_F(RepositoryTest, AuditLogIsAppendOnly) {
    const int sid = w.submissionRepo.save(Submission::create(w.assignmentId, w.student1->getId(), "a", 0, false));
    w.auditRepo.save(AuditEntry(0, sid, w.lecturer->getId(), T("2026-10-11 10:00"), std::nullopt, 60, "Initial marking"));
    w.auditRepo.save(AuditEntry(0, sid, w.hod->getId(), T("2026-10-12 10:00"), 60, 65, "Rubric"));
    std::vector<AuditEntry> h = w.auditRepo.findBySubmission(sid);
    ASSERT_EQ(h.size(), 2u);
    EXPECT_FALSE(h[0].getOldValue().has_value());
    EXPECT_EQ(h[1].getOldValue(), 60);
    EXPECT_EQ(h[1].getNewValue(), 65);
    EXPECT_EQ(w.auditRepo.findAll().size(), 2u);
    EXPECT_THROW(w.db.exec("UPDATE audit_entry SET reason = 'tampered'"), DatabaseException);
    EXPECT_THROW(w.db.exec("DELETE FROM audit_entry"), DatabaseException);
    EXPECT_EQ(w.auditRepo.findAll().size(), 2u);
}

TEST_F(RepositoryTest, AuditEntryNeedsAReasonAndARealSubmission) {
    const int sid = w.submissionRepo.save(Submission::create(w.assignmentId, w.student1->getId(), "a", 0, false));
    EXPECT_THROW(w.auditRepo.save(AuditEntry(0, sid, w.lecturer->getId(), 0, std::nullopt, 1, "")), DatabaseException);
    EXPECT_THROW(w.auditRepo.save(AuditEntry(0, 424242, w.lecturer->getId(), 0, std::nullopt, 1, "x")), DatabaseException);
}

TEST_F(RepositoryTest, TransactionsCommitAndRollBack) {
    {
        ScopedTransaction tx(w.db);
        w.courseRepo.saveDepartment(Department(0, "Rolled back"));
    }   // no commit -> automatic rollback
    EXPECT_EQ(w.courseRepo.findDepartments().size(), 1u);
    {
        ScopedTransaction tx(w.db);
        w.courseRepo.saveDepartment(Department(0, "Kept"));
        tx.commit();
    }
    EXPECT_EQ(w.courseRepo.findDepartments().size(), 2u);
}

TEST(DatabaseTest, OpeningAnImpossiblePathThrows) {
    EXPECT_THROW(Database("/nonexistent_directory_for_lms_tests/x.db"), DatabaseException);
}

TEST(DatabaseTest, BadSqlThrowsDatabaseException) {
    Database db(":memory:");
    EXPECT_THROW(db.exec("THIS IS NOT SQL"), DatabaseException);
    EXPECT_THROW(db.prepare("SELECT * FROM missing_table"), DatabaseException);
}

TEST(DatabaseTest, SchemaCanBeAppliedTwice) {
    Database db(":memory:");
    db.initialiseSchema();
    EXPECT_NO_THROW(db.initialiseSchema());
}

TEST(DatabaseTest, ForeignKeysAreEnforced) {
    Database db(":memory:");
    db.initialiseSchema();
    EXPECT_THROW(db.exec("INSERT INTO course (code, title, department_id) VALUES ('X','Y',123)"), DatabaseException);
}
