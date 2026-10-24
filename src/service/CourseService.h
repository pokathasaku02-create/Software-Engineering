#pragma once
#include <string>
#include <vector>

#include "../data/Repositories.h"
#include "../domain/IClock.h"

namespace lms {

struct CourseData {
    std::string code;
    std::string title;
    int departmentId = 0;
};

class CourseService {
public:
    CourseService(ICourseRepository& courses, IUserRepository& users, IClock& clock)
        : courses_(courses), users_(users), clock_(clock) {}

    int createDepartment(const User& actor, const std::string& name);
    std::vector<Department> departments();
    int createCourse(const User& actor, const CourseData& data);
    void assignLecturer(const User& actor, int courseId, int lecturerId);
    void enrolStudent(const User& actor, int courseId, int studentId);
    // The courses relevant to this user: enrolled (student), taught (lecturer),
    // in own department (head of department) or all (administrator).
    std::vector<Course> coursesFor(const User& user);

private:
    ICourseRepository& courses_;
    IUserRepository& users_;
    IClock& clock_;
};

}  // namespace lms
