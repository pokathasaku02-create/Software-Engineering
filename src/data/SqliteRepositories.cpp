#include "SqliteRepositories.h"

#include "../domain/UserFactory.h"

namespace lms {
namespace {

// ---- row mappers (column order matches the SELECT lists below) ----

const char* kUserCols = "user_id, username, password_hash, salt, full_name, role, ref_number, department_id, is_active";

std::unique_ptr<User> userFromRow(const Statement& s) {
    UserDetails d;
    d.id = static_cast<int>(s.columnInt(0));
    d.username = s.columnText(1);
    d.passwordHash = s.columnText(2);
    d.salt = s.columnText(3);
    d.fullName = s.columnText(4);
    d.referenceNumber = s.columnText(6);
    d.departmentId = s.columnOptInt(7);
    d.active = s.columnInt(8) != 0;
    return UserFactory::create(roleFromString(s.columnText(5)), d);
}

const char* kCourseCols = "course_id, code, title, department_id, lecturer_id";

Course courseFromRow(const Statement& s) {
    return Course(static_cast<int>(s.columnInt(0)), s.columnText(1), s.columnText(2),
                  static_cast<int>(s.columnInt(3)), s.columnOptInt(4));
}

const char* kAssignmentCols = "assignment_id, course_id, title, description, deadline, max_mark";

Assignment assignmentFromRow(const Statement& s) {
    return Assignment(static_cast<int>(s.columnInt(0)), static_cast<int>(s.columnInt(1)), s.columnText(2),
                      s.columnText(3), parseDateTime(s.columnText(4)), static_cast<int>(s.columnInt(5)));
}

const char* kSubmissionCols =
    "s.submission_id, s.assignment_id, s.student_id, s.content, s.submitted_at, s.resubmit_count, s.status, "
    "s.raw_mark, s.final_mark, s.feedback";

Submission submissionFromRow(const Statement& s) {
    return Submission(static_cast<int>(s.columnInt(0)), static_cast<int>(s.columnInt(1)),
                      static_cast<int>(s.columnInt(2)), s.columnText(3), parseDateTime(s.columnText(4)),
                      static_cast<int>(s.columnInt(5)), statusFromString(s.columnText(6)), s.columnOptInt(7),
                      s.columnOptInt(8), s.columnText(9));
}

}  // namespace

// ===================== users =====================

std::unique_ptr<User> SqliteUserRepository::findById(int id) {
    Statement s = db_.prepare(std::string("SELECT ") + kUserCols + " FROM users WHERE user_id = ?");
    s.bind(1, id);
    return s.step() ? userFromRow(s) : nullptr;
}

std::unique_ptr<User> SqliteUserRepository::findByUsername(const std::string& username) {
    Statement s = db_.prepare(std::string("SELECT ") + kUserCols + " FROM users WHERE username = ?");
    s.bind(1, username);
    return s.step() ? userFromRow(s) : nullptr;
}

std::vector<std::unique_ptr<User>> SqliteUserRepository::findAll() {
    Statement s = db_.prepare(std::string("SELECT ") + kUserCols + " FROM users ORDER BY user_id");
    std::vector<std::unique_ptr<User>> out;
    while (s.step()) out.push_back(userFromRow(s));
    return out;
}

int SqliteUserRepository::save(const User& u) {
    Statement s = db_.prepare(
        "INSERT INTO users (username, password_hash, salt, full_name, role, ref_number, department_id, is_active) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    s.bind(1, u.getUsername()).bind(2, u.getPasswordHash()).bind(3, u.getSalt()).bind(4, u.getFullName())
        .bind(5, toString(u.getRole())).bind(6, u.getReferenceNumber()).bind(7, u.getDepartmentId())
        .bind(8, u.isActive() ? 1 : 0);
    s.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteUserRepository::update(const User& u) {
    Statement s = db_.prepare("UPDATE users SET password_hash = ?, salt = ?, full_name = ?, is_active = ? WHERE user_id = ?");
    s.bind(1, u.getPasswordHash()).bind(2, u.getSalt()).bind(3, u.getFullName()).bind(4, u.isActive() ? 1 : 0)
        .bind(5, u.getId());
    s.run();
}

// ===================== courses & departments =====================

std::optional<Course> SqliteCourseRepository::findById(int id) {
    Statement s = db_.prepare(std::string("SELECT ") + kCourseCols + " FROM course WHERE course_id = ?");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return courseFromRow(s);
}

std::vector<Course> SqliteCourseRepository::findAll() {
    Statement s = db_.prepare(std::string("SELECT ") + kCourseCols + " FROM course ORDER BY code");
    std::vector<Course> out;
    while (s.step()) out.push_back(courseFromRow(s));
    return out;
}

std::vector<Course> SqliteCourseRepository::findByDepartment(int departmentId) {
    Statement s = db_.prepare(std::string("SELECT ") + kCourseCols + " FROM course WHERE department_id = ? ORDER BY code");
    s.bind(1, departmentId);
    std::vector<Course> out;
    while (s.step()) out.push_back(courseFromRow(s));
    return out;
}

std::vector<Course> SqliteCourseRepository::findByStudent(int studentId) {
    Statement s = db_.prepare(
        "SELECT c.course_id, c.code, c.title, c.department_id, c.lecturer_id FROM course c "
        "JOIN enrolment e ON e.course_id = c.course_id WHERE e.student_id = ? ORDER BY c.code");
    s.bind(1, studentId);
    std::vector<Course> out;
    while (s.step()) out.push_back(courseFromRow(s));
    return out;
}

int SqliteCourseRepository::save(const Course& c) {
    Statement s = db_.prepare("INSERT INTO course (code, title, department_id, lecturer_id) VALUES (?, ?, ?, ?)");
    s.bind(1, c.getCode()).bind(2, c.getTitle()).bind(3, c.getDepartmentId()).bind(4, c.getLecturerId());
    s.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteCourseRepository::update(const Course& c) {
    Statement s = db_.prepare("UPDATE course SET title = ?, lecturer_id = ? WHERE course_id = ?");
    s.bind(1, c.getTitle()).bind(2, c.getLecturerId()).bind(3, c.getId());
    s.run();
}

std::optional<Department> SqliteCourseRepository::findDepartment(int id) {
    Statement s = db_.prepare("SELECT department_id, name FROM department WHERE department_id = ?");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return Department(static_cast<int>(s.columnInt(0)), s.columnText(1));
}

std::vector<Department> SqliteCourseRepository::findDepartments() {
    Statement s = db_.prepare("SELECT department_id, name FROM department ORDER BY name");
    std::vector<Department> out;
    while (s.step()) out.emplace_back(static_cast<int>(s.columnInt(0)), s.columnText(1));
    return out;
}

int SqliteCourseRepository::saveDepartment(const Department& d) {
    Statement s = db_.prepare("INSERT INTO department (name) VALUES (?)");
    s.bind(1, d.getName());
    s.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteCourseRepository::enrol(const Enrolment& e) {
    Statement s = db_.prepare("INSERT INTO enrolment (student_id, course_id, enrolled_on) VALUES (?, ?, ?)");
    s.bind(1, e.getStudentId()).bind(2, e.getCourseId()).bind(3, formatDateTime(e.getEnrolledOn()));
    s.run();
}

// ===================== assignments & extensions =====================

std::optional<Assignment> SqliteAssignmentRepository::findById(int id) {
    Statement s = db_.prepare(std::string("SELECT ") + kAssignmentCols + " FROM assignment WHERE assignment_id = ?");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return assignmentFromRow(s);
}

std::vector<Assignment> SqliteAssignmentRepository::findByCourse(int courseId) {
    Statement s = db_.prepare(std::string("SELECT ") + kAssignmentCols +
                              " FROM assignment WHERE course_id = ? ORDER BY deadline, assignment_id");
    s.bind(1, courseId);
    std::vector<Assignment> out;
    while (s.step()) out.push_back(assignmentFromRow(s));
    return out;
}

int SqliteAssignmentRepository::save(const Assignment& a) {
    Statement s = db_.prepare(
        "INSERT INTO assignment (course_id, title, description, deadline, max_mark) VALUES (?, ?, ?, ?, ?)");
    s.bind(1, a.getCourseId()).bind(2, a.getTitle()).bind(3, a.getDescription())
        .bind(4, formatDateTime(a.getDeadline())).bind(5, a.getMaxMark());
    s.run();
    return static_cast<int>(db_.lastInsertId());
}

std::optional<Extension> SqliteAssignmentRepository::findExtension(int studentId, int assignmentId) {
    Statement s = db_.prepare(
        "SELECT extension_id, assignment_id, student_id, new_deadline, reason, granted_by, granted_at "
        "FROM extension WHERE student_id = ? AND assignment_id = ?");
    s.bind(1, studentId).bind(2, assignmentId);
    if (!s.step()) return std::nullopt;
    return Extension(static_cast<int>(s.columnInt(0)), static_cast<int>(s.columnInt(1)),
                     static_cast<int>(s.columnInt(2)), parseDateTime(s.columnText(3)), s.columnText(4),
                     static_cast<int>(s.columnInt(5)), parseDateTime(s.columnText(6)));
}

int SqliteAssignmentRepository::saveExtension(const Extension& e) {
    Statement s = db_.prepare(
        "INSERT INTO extension (assignment_id, student_id, new_deadline, reason, granted_by, granted_at) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(assignment_id, student_id) DO UPDATE SET new_deadline = excluded.new_deadline, "
        "reason = excluded.reason, granted_by = excluded.granted_by, granted_at = excluded.granted_at");
    s.bind(1, e.getAssignmentId()).bind(2, e.getStudentId()).bind(3, formatDateTime(e.getNewDeadline()))
        .bind(4, e.getReason()).bind(5, e.getGrantedBy()).bind(6, formatDateTime(e.getGrantedAt()));
    s.run();
    Statement id = db_.prepare("SELECT extension_id FROM extension WHERE assignment_id = ? AND student_id = ?");
    id.bind(1, e.getAssignmentId()).bind(2, e.getStudentId());
    id.step();
    return static_cast<int>(id.columnInt(0));
}

// ===================== submissions =====================

std::optional<Submission> SqliteSubmissionRepository::findById(int id) {
    Statement s = db_.prepare(std::string("SELECT ") + kSubmissionCols + " FROM submission s WHERE s.submission_id = ?");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return submissionFromRow(s);
}

std::optional<Submission> SqliteSubmissionRepository::findByStudentAndAssignment(int studentId, int assignmentId) {
    Statement s = db_.prepare(std::string("SELECT ") + kSubmissionCols +
                              " FROM submission s WHERE s.student_id = ? AND s.assignment_id = ?");
    s.bind(1, studentId).bind(2, assignmentId);
    if (!s.step()) return std::nullopt;
    return submissionFromRow(s);
}

std::vector<Submission> SqliteSubmissionRepository::findByCourseAndStatus(int courseId, SubmissionStatus status) {
    Statement s = db_.prepare(std::string("SELECT ") + kSubmissionCols +
                              " FROM submission s JOIN assignment a ON a.assignment_id = s.assignment_id "
                              "WHERE a.course_id = ? AND s.status = ? ORDER BY s.assignment_id, s.student_id");
    s.bind(1, courseId).bind(2, toString(status));
    std::vector<Submission> out;
    while (s.step()) out.push_back(submissionFromRow(s));
    return out;
}

std::vector<Submission> SqliteSubmissionRepository::findReleasedByStudent(int studentId) {
    Statement s = db_.prepare(std::string("SELECT ") + kSubmissionCols +
                              " FROM submission s WHERE s.student_id = ? AND s.status = 'RELEASED' "
                              "ORDER BY s.assignment_id");
    s.bind(1, studentId);
    std::vector<Submission> out;
    while (s.step()) out.push_back(submissionFromRow(s));
    return out;
}

int SqliteSubmissionRepository::save(const Submission& m) {
    Statement s = db_.prepare(
        "INSERT INTO submission (assignment_id, student_id, content, submitted_at, resubmit_count, status, "
        "raw_mark, final_mark, feedback) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    s.bind(1, m.getAssignmentId()).bind(2, m.getStudentId()).bind(3, m.getContent())
        .bind(4, formatDateTime(m.getSubmittedAt())).bind(5, m.getResubmitCount()).bind(6, toString(m.getStatus()))
        .bind(7, m.getRawMark()).bind(8, m.getFinalMark()).bind(9, m.getFeedback());
    s.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteSubmissionRepository::update(const Submission& m) {
    Statement s = db_.prepare(
        "UPDATE submission SET content = ?, submitted_at = ?, resubmit_count = ?, status = ?, raw_mark = ?, "
        "final_mark = ?, feedback = ? WHERE submission_id = ?");
    s.bind(1, m.getContent()).bind(2, formatDateTime(m.getSubmittedAt())).bind(3, m.getResubmitCount())
        .bind(4, toString(m.getStatus())).bind(5, m.getRawMark()).bind(6, m.getFinalMark())
        .bind(7, m.getFeedback()).bind(8, m.getId());
    s.run();
}

// ===================== audit =====================

namespace {
AuditEntry auditFromRow(const Statement& s) {
    return AuditEntry(static_cast<int>(s.columnInt(0)), static_cast<int>(s.columnInt(1)),
                      static_cast<int>(s.columnInt(2)), parseDateTime(s.columnText(3)), s.columnOptInt(4),
                      static_cast<int>(s.columnInt(5)), s.columnText(6));
}
const char* kAuditCols = "audit_id, submission_id, changed_by, changed_at, old_value, new_value, reason";
}  // namespace

int SqliteAuditRepository::save(const AuditEntry& e) {
    Statement s = db_.prepare(
        "INSERT INTO audit_entry (submission_id, changed_by, changed_at, old_value, new_value, reason) "
        "VALUES (?, ?, ?, ?, ?, ?)");
    s.bind(1, e.getSubmissionId()).bind(2, e.getChangedBy()).bind(3, formatDateTime(e.getChangedAt()))
        .bind(4, e.getOldValue()).bind(5, e.getNewValue()).bind(6, e.getReason());
    s.run();
    return static_cast<int>(db_.lastInsertId());
}

std::vector<AuditEntry> SqliteAuditRepository::findBySubmission(int submissionId) {
    Statement s = db_.prepare(std::string("SELECT ") + kAuditCols + " FROM audit_entry WHERE submission_id = ? ORDER BY audit_id");
    s.bind(1, submissionId);
    std::vector<AuditEntry> out;
    while (s.step()) out.push_back(auditFromRow(s));
    return out;
}

std::vector<AuditEntry> SqliteAuditRepository::findAll() {
    Statement s = db_.prepare(std::string("SELECT ") + kAuditCols + " FROM audit_entry ORDER BY audit_id");
    std::vector<AuditEntry> out;
    while (s.step()) out.push_back(auditFromRow(s));
    return out;
}

}  // namespace lms
