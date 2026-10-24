#include "AssignmentService.h"

#include "Authorization.h"

namespace lms {

Course AssignmentService::requireTaughtBy(const User& lecturer, int courseId) {
    std::optional<Course> course = courses_.findById(courseId);
    if (!course) throw NotFoundException("No such course.");
    if (course->getLecturerId() != lecturer.getId()) throw AccessDeniedException("You do not teach that course.");
    return *course;
}

int AssignmentService::createAssignment(const User& lecturer, const AssignmentData& data) {
    requirePermission(lecturer, Permission::CreateAssignment, "create assignments");
    requireTaughtBy(lecturer, data.courseId);
    if (data.title.empty() || data.title.size() > 100) throw ValidationException("The title must have 1 to 100 characters.");
    if (data.description.size() > 2000) throw ValidationException("The description is too long (maximum 2000 characters).");
    if (data.maxMark < 1 || data.maxMark > 1000) throw ValidationException("The maximum mark must be between 1 and 1000.");
    if (data.deadline <= clock_.now()) throw ValidationException("The deadline must be in the future.");
    return assignments_.save(Assignment(0, data.courseId, data.title, data.description, data.deadline, data.maxMark));
}

int AssignmentService::grantExtension(const User& lecturer, const ExtensionData& data) {
    requirePermission(lecturer, Permission::GrantExtension, "grant extensions");
    std::optional<Assignment> a = assignments_.findById(data.assignmentId);
    if (!a) throw NotFoundException("No such assignment.");
    requireTaughtBy(lecturer, a->getCourseId());
    bool enrolled = false;
    for (const Course& c : courses_.findByStudent(data.studentId)) enrolled = enrolled || c.getId() == a->getCourseId();
    if (!enrolled) throw ValidationException("That student is not enrolled in the course.");
    if (data.newDeadline <= a->getDeadline()) throw ValidationException("The extended deadline must be later than the assignment deadline.");
    if (data.reason.empty() || data.reason.size() > 500) throw ValidationException("Give a reason of 1 to 500 characters.");
    return assignments_.saveExtension(
        Extension(0, data.assignmentId, data.studentId, data.newDeadline, data.reason, lecturer.getId(), clock_.now()));
}

std::vector<Assignment> AssignmentService::assignmentsFor(const User& student) {
    requirePermission(student, Permission::SubmitWork, "view your assignments");
    std::vector<Assignment> all;
    for (const Course& c : courses_.findByStudent(student.getId()))
        for (const Assignment& a : assignments_.findByCourse(c.getId())) all.push_back(a);
    return all;
}

std::vector<Assignment> AssignmentService::assignmentsForCourse(const User& actor, int courseId) {
    std::optional<Course> course = courses_.findById(courseId);
    if (!course) throw NotFoundException("No such course.");
    const bool teaches = actor.getRole() == Role::Lecturer && course->getLecturerId() == actor.getId();
    const bool heads = actor.getRole() == Role::HeadOfDepartment && actor.getDepartmentId() == course->getDepartmentId();
    if (!actor.isActive() || !(teaches || heads)) throw AccessDeniedException("You may not view that course's assignments.");
    return assignments_.findByCourse(courseId);
}

Assignment AssignmentService::getAssignment(int assignmentId) {
    std::optional<Assignment> a = assignments_.findById(assignmentId);
    if (!a) throw NotFoundException("No such assignment.");
    return *a;
}

}  // namespace lms
