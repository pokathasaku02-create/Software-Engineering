#include "CourseService.h"

#include "Authorization.h"

namespace lms {

int CourseService::createDepartment(const User& actor, const std::string& name) {
    requirePermission(actor, Permission::ManageCourses, "manage departments");
    if (name.empty() || name.size() > 100) throw ValidationException("The department name must have 1 to 100 characters.");
    for (const Department& d : courses_.findDepartments())
        if (d.getName() == name) throw ValidationException("That department already exists.");
    return courses_.saveDepartment(Department(0, name));
}

std::vector<Department> CourseService::departments() { return courses_.findDepartments(); }

int CourseService::createCourse(const User& actor, const CourseData& data) {
    requirePermission(actor, Permission::ManageCourses, "manage courses");
    if (data.code.empty() || data.code.size() > 20) throw ValidationException("The course code must have 1 to 20 characters.");
    if (data.title.empty() || data.title.size() > 100) throw ValidationException("The course title must have 1 to 100 characters.");
    if (!courses_.findDepartment(data.departmentId)) throw NotFoundException("No such department.");
    for (const Course& c : courses_.findAll())
        if (c.getCode() == data.code) throw ValidationException("That course code already exists.");
    return courses_.save(Course(0, data.code, data.title, data.departmentId, std::nullopt));
}

void CourseService::assignLecturer(const User& actor, int courseId, int lecturerId) {
    requirePermission(actor, Permission::ManageCourses, "assign lecturers");
    std::optional<Course> course = courses_.findById(courseId);
    if (!course) throw NotFoundException("No such course.");
    std::unique_ptr<User> lecturer = users_.findById(lecturerId);
    if (!lecturer || lecturer->getRole() != Role::Lecturer) throw ValidationException("That user is not a lecturer.");
    if (!lecturer->isActive()) throw ValidationException("That lecturer's account is disabled.");
    if (lecturer->getDepartmentId() != course->getDepartmentId())
        throw ValidationException("The lecturer belongs to a different department than the course.");
    course->setLecturerId(lecturerId);
    courses_.update(*course);
}

void CourseService::enrolStudent(const User& actor, int courseId, int studentId) {
    requirePermission(actor, Permission::ManageCourses, "enrol students");
    if (!courses_.findById(courseId)) throw NotFoundException("No such course.");
    std::unique_ptr<User> student = users_.findById(studentId);
    if (!student || student->getRole() != Role::Student) throw ValidationException("That user is not a student.");
    for (const Course& c : courses_.findByStudent(studentId))
        if (c.getId() == courseId) throw ValidationException("The student is already enrolled in that course.");
    courses_.enrol(Enrolment(studentId, courseId, clock_.now()));
}

std::vector<Course> CourseService::coursesFor(const User& user) {
    switch (user.getRole()) {
        case Role::Student: return courses_.findByStudent(user.getId());
        case Role::HeadOfDepartment: return courses_.findByDepartment(*user.getDepartmentId());
        case Role::Lecturer: {
            std::vector<Course> mine;
            for (const Course& c : courses_.findByDepartment(*user.getDepartmentId()))
                if (c.getLecturerId() == user.getId()) mine.push_back(c);
            return mine;
        }
        case Role::Administrator: return courses_.findAll();
    }
    return {};
}

}  // namespace lms
