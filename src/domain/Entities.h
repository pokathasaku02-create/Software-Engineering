// Plain domain entities: Department, Course, Enrolment, Assignment, Extension, AuditEntry.
// Each has private state and read-only getters; ids are assigned by the repositories.
#pragma once
#include <optional>
#include <string>
#include <utility>

#include "Types.h"

namespace lms {

class Department {
public:
    Department(int id, std::string name) : id_(id), name_(std::move(name)) {}
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    void setId(int id) { id_ = id; }
private:
    int id_;
    std::string name_;
};

class Course {
public:
    Course(int id, std::string code, std::string title, int departmentId, std::optional<int> lecturerId)
        : id_(id), code_(std::move(code)), title_(std::move(title)),
          departmentId_(departmentId), lecturerId_(lecturerId) {}
    int getId() const { return id_; }
    const std::string& getCode() const { return code_; }
    const std::string& getTitle() const { return title_; }
    int getDepartmentId() const { return departmentId_; }
    std::optional<int> getLecturerId() const { return lecturerId_; }
    void setId(int id) { id_ = id; }
    void setLecturerId(std::optional<int> id) { lecturerId_ = id; }
private:
    int id_;
    std::string code_, title_;
    int departmentId_;
    std::optional<int> lecturerId_;
};

class Enrolment {
public:
    Enrolment(int studentId, int courseId, DateTime enrolledOn)
        : studentId_(studentId), courseId_(courseId), enrolledOn_(enrolledOn) {}
    int getStudentId() const { return studentId_; }
    int getCourseId() const { return courseId_; }
    DateTime getEnrolledOn() const { return enrolledOn_; }
private:
    int studentId_, courseId_;
    DateTime enrolledOn_;
};

class Assignment {
public:
    Assignment(int id, int courseId, std::string title, std::string description, DateTime deadline, int maxMark)
        : id_(id), courseId_(courseId), title_(std::move(title)), description_(std::move(description)),
          deadline_(deadline), maxMark_(maxMark) {}
    int getId() const { return id_; }
    int getCourseId() const { return courseId_; }
    const std::string& getTitle() const { return title_; }
    const std::string& getDescription() const { return description_; }
    DateTime getDeadline() const { return deadline_; }
    int getMaxMark() const { return maxMark_; }
    bool isOpenAt(DateTime t) const { return t <= deadline_; }
    void setId(int id) { id_ = id; }
private:
    int id_, courseId_;
    std::string title_, description_;
    DateTime deadline_;
    int maxMark_;
};

class Extension {
public:
    Extension(int id, int assignmentId, int studentId, DateTime newDeadline, std::string reason, int grantedBy,
              DateTime grantedAt)
        : id_(id), assignmentId_(assignmentId), studentId_(studentId), newDeadline_(newDeadline),
          reason_(std::move(reason)), grantedBy_(grantedBy), grantedAt_(grantedAt) {}
    int getId() const { return id_; }
    int getAssignmentId() const { return assignmentId_; }
    int getStudentId() const { return studentId_; }
    DateTime getNewDeadline() const { return newDeadline_; }
    const std::string& getReason() const { return reason_; }
    int getGrantedBy() const { return grantedBy_; }
    DateTime getGrantedAt() const { return grantedAt_; }
    void setId(int id) { id_ = id; }
private:
    int id_, assignmentId_, studentId_;
    DateTime newDeadline_;
    std::string reason_;
    int grantedBy_;
    DateTime grantedAt_;
};

// Append-only record of one mark change: who, when, old value, new value, why.
class AuditEntry {
public:
    AuditEntry(int id, int submissionId, int changedBy, DateTime changedAt, std::optional<int> oldValue,
               int newValue, std::string reason)
        : id_(id), submissionId_(submissionId), changedBy_(changedBy), changedAt_(changedAt),
          oldValue_(oldValue), newValue_(newValue), reason_(std::move(reason)) {}
    int getId() const { return id_; }
    int getSubmissionId() const { return submissionId_; }
    int getChangedBy() const { return changedBy_; }
    DateTime getChangedAt() const { return changedAt_; }
    std::optional<int> getOldValue() const { return oldValue_; }
    int getNewValue() const { return newValue_; }
    const std::string& getReason() const { return reason_; }
    void setId(int id) { id_ = id; }
private:
    int id_, submissionId_, changedBy_;
    DateTime changedAt_;
    std::optional<int> oldValue_;
    int newValue_;
    std::string reason_;
};

}  // namespace lms
