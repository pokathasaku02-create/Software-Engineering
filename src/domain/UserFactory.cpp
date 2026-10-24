#include "UserFactory.h"

#include "Exceptions.h"

namespace lms {

std::unique_ptr<User> UserFactory::create(Role role, const UserDetails& d) {
    const bool staff = role == Role::Lecturer || role == Role::HeadOfDepartment;
    if (staff && !d.departmentId) throw ValidationException("A lecturer or head of department needs a department.");
    if ((staff || role == Role::Student) && d.referenceNumber.empty())
        throw ValidationException("A student number or staff number is required.");
    switch (role) {
        case Role::Student:
            return std::make_unique<Student>(d.id, d.username, d.passwordHash, d.salt, d.fullName, d.active,
                                             d.referenceNumber);
        case Role::Lecturer:
            return std::make_unique<Lecturer>(d.id, d.username, d.passwordHash, d.salt, d.fullName, d.active,
                                              d.referenceNumber, *d.departmentId);
        case Role::HeadOfDepartment:
            return std::make_unique<HeadOfDepartment>(d.id, d.username, d.passwordHash, d.salt, d.fullName, d.active,
                                                      d.referenceNumber, *d.departmentId);
        case Role::Administrator:
            return std::make_unique<Administrator>(d.id, d.username, d.passwordHash, d.salt, d.fullName, d.active);
    }
    throw ValidationException("Unknown role.");
}

}  // namespace lms
