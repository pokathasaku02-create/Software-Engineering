#include "User.h"

namespace lms {

bool Student::canPerform(Permission p) const {
    return p == Permission::SubmitWork || p == Permission::ViewReleasedMarks;
}

bool Lecturer::canPerform(Permission p) const {
    return p == Permission::CreateAssignment || p == Permission::GrantExtension || p == Permission::MarkSubmission;
}

bool HeadOfDepartment::canPerform(Permission p) const {
    return p == Permission::ModerateMarks || p == Permission::ReleaseMarks;
}

bool Administrator::canPerform(Permission p) const {
    return p == Permission::ManageUsers || p == Permission::ManageCourses;
}

}  // namespace lms
