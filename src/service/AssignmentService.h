#pragma once
#include <string>
#include <vector>

#include "../data/Repositories.h"
#include "../domain/IClock.h"

namespace lms {

struct AssignmentData {
    int courseId = 0;
    std::string title;
    std::string description;
    DateTime deadline = 0;
    int maxMark = 100;
};

struct ExtensionData {
    int assignmentId = 0;
    int studentId = 0;
    DateTime newDeadline = 0;
    std::string reason;
};

class AssignmentService {
public:
    AssignmentService(IAssignmentRepository& assignments, ICourseRepository& courses, IClock& clock)
        : assignments_(assignments), courses_(courses), clock_(clock) {}

    int createAssignment(const User& lecturer, const AssignmentData& data);
    int grantExtension(const User& lecturer, const ExtensionData& data);
    std::vector<Assignment> assignmentsFor(const User& student);
    std::vector<Assignment> assignmentsForCourse(const User& actor, int courseId);
    Assignment getAssignment(int assignmentId);

private:
    Course requireTaughtBy(const User& lecturer, int courseId);

    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    IClock& clock_;
};

}  // namespace lms
