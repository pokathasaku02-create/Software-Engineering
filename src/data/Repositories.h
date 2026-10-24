// Repository pattern: the interfaces the services depend on. Nothing here mentions SQLite.
#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "../domain/Entities.h"
#include "../domain/Submission.h"
#include "../domain/User.h"

namespace lms {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual std::unique_ptr<User> findById(int id) = 0;
    virtual std::unique_ptr<User> findByUsername(const std::string& username) = 0;
    virtual std::vector<std::unique_ptr<User>> findAll() = 0;
    virtual int save(const User& user) = 0;
    virtual void update(const User& user) = 0;
};

class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;
    virtual std::optional<Course> findById(int id) = 0;
    virtual std::vector<Course> findAll() = 0;
    virtual std::vector<Course> findByDepartment(int departmentId) = 0;
    virtual std::vector<Course> findByStudent(int studentId) = 0;
    virtual int save(const Course& course) = 0;
    virtual void update(const Course& course) = 0;
    virtual std::optional<Department> findDepartment(int id) = 0;
    virtual std::vector<Department> findDepartments() = 0;
    virtual int saveDepartment(const Department& department) = 0;
    virtual void enrol(const Enrolment& enrolment) = 0;
};

class IAssignmentRepository {
public:
    virtual ~IAssignmentRepository() = default;
    virtual std::optional<Assignment> findById(int id) = 0;
    virtual std::vector<Assignment> findByCourse(int courseId) = 0;
    virtual int save(const Assignment& assignment) = 0;
    virtual std::optional<Extension> findExtension(int studentId, int assignmentId) = 0;
    // Inserts, or replaces the student's existing extension for that assignment.
    virtual int saveExtension(const Extension& extension) = 0;
};

class ISubmissionRepository {
public:
    virtual ~ISubmissionRepository() = default;
    virtual std::optional<Submission> findById(int id) = 0;
    virtual std::optional<Submission> findByStudentAndAssignment(int studentId, int assignmentId) = 0;
    virtual std::vector<Submission> findByCourseAndStatus(int courseId, SubmissionStatus status) = 0;
    virtual std::vector<Submission> findReleasedByStudent(int studentId) = 0;
    virtual int save(const Submission& submission) = 0;
    virtual void update(const Submission& submission) = 0;
};

class IAuditRepository {
public:
    virtual ~IAuditRepository() = default;
    virtual int save(const AuditEntry& entry) = 0;
    virtual std::vector<AuditEntry> findBySubmission(int submissionId) = 0;
    virtual std::vector<AuditEntry> findAll() = 0;
};

}  // namespace lms
