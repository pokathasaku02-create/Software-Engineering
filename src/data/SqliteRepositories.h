// SQLite realisations of the repository interfaces. All queries are parameterised.
#pragma once
#include "Database.h"
#include "Repositories.h"

namespace lms {

class SqliteUserRepository : public IUserRepository {
public:
    explicit SqliteUserRepository(Database& db) : db_(db) {}
    std::unique_ptr<User> findById(int id) override;
    std::unique_ptr<User> findByUsername(const std::string& username) override;
    std::vector<std::unique_ptr<User>> findAll() override;
    int save(const User& user) override;
    void update(const User& user) override;
private:
    Database& db_;
};

class SqliteCourseRepository : public ICourseRepository {
public:
    explicit SqliteCourseRepository(Database& db) : db_(db) {}
    std::optional<Course> findById(int id) override;
    std::vector<Course> findAll() override;
    std::vector<Course> findByDepartment(int departmentId) override;
    std::vector<Course> findByStudent(int studentId) override;
    int save(const Course& course) override;
    void update(const Course& course) override;
    std::optional<Department> findDepartment(int id) override;
    std::vector<Department> findDepartments() override;
    int saveDepartment(const Department& department) override;
    void enrol(const Enrolment& enrolment) override;
private:
    Database& db_;
};

class SqliteAssignmentRepository : public IAssignmentRepository {
public:
    explicit SqliteAssignmentRepository(Database& db) : db_(db) {}
    std::optional<Assignment> findById(int id) override;
    std::vector<Assignment> findByCourse(int courseId) override;
    int save(const Assignment& assignment) override;
    std::optional<Extension> findExtension(int studentId, int assignmentId) override;
    int saveExtension(const Extension& extension) override;
private:
    Database& db_;
};

class SqliteSubmissionRepository : public ISubmissionRepository {
public:
    explicit SqliteSubmissionRepository(Database& db) : db_(db) {}
    std::optional<Submission> findById(int id) override;
    std::optional<Submission> findByStudentAndAssignment(int studentId, int assignmentId) override;
    std::vector<Submission> findByCourseAndStatus(int courseId, SubmissionStatus status) override;
    std::vector<Submission> findReleasedByStudent(int studentId) override;
    int save(const Submission& submission) override;
    void update(const Submission& submission) override;
private:
    Database& db_;
};

class SqliteAuditRepository : public IAuditRepository {
public:
    explicit SqliteAuditRepository(Database& db) : db_(db) {}
    int save(const AuditEntry& entry) override;
    std::vector<AuditEntry> findBySubmission(int submissionId) override;
    std::vector<AuditEntry> findAll() override;
private:
    Database& db_;
};

}  // namespace lms
