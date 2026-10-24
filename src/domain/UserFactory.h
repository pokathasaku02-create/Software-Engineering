// Factory pattern: the only place that constructs concrete User subclasses.
#pragma once
#include <memory>
#include <optional>
#include <string>

#include "User.h"

namespace lms {

struct UserDetails {
    int id = 0;
    std::string username;
    std::string passwordHash;
    std::string salt;
    std::string fullName;
    std::string referenceNumber;       // student number or staff number
    std::optional<int> departmentId;   // required for lecturers and heads of department
    bool active = true;
};

class UserFactory {
public:
    static std::unique_ptr<User> create(Role role, const UserDetails& details);
};

}  // namespace lms
