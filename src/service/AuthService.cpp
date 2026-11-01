#include "AuthService.h"

#include <cctype>

#include "Authorization.h"

namespace lms {

AuthService::AuthService(IUserRepository& users, const PasswordHasher& hasher) : users_(users), hasher_(hasher) {
    dummySalt_ = hasher_.generateSalt();
    dummyHash_ = hasher_.hash("not-a-real-password", dummySalt_);
}

const User* AuthService::login(const std::string& username, const std::string& password) {
    std::unique_ptr<User> user = users_.findByUsername(username);
    const bool ok = user ? hasher_.verify(password, user->getSalt(), user->getPasswordHash())
                         : (hasher_.verify(password, dummySalt_, dummyHash_), false);
    if (!ok || !user->isActive()) throw AuthenticationException("Invalid username or password.");
    current_ = std::move(user);
    return current_.get();
}

void AuthService::logout() { current_.reset(); }

bool AuthService::hasAnyUsers() { return !users_.findAll().empty(); }

void AuthService::validatePassword(const std::string& password) {
    if (password.size() < 8) throw ValidationException("The password must have at least 8 characters.");
    if (password.size() > 128) throw ValidationException("The password is too long (maximum 128 characters).");
}

void AuthService::validateUsername(const std::string& username) {
    if (username.size() < 3 || username.size() > 32)
        throw ValidationException("The username must have 3 to 32 characters.");
    for (unsigned char c : username)
        if (!std::isalnum(c) && c != '_' && c != '.' && c != '-')
            throw ValidationException("The username may contain only letters, digits, '_', '.' and '-'.");
}

int AuthService::createInitialAdministrator(const std::string& username, const std::string& fullName,
                                            const std::string& password) {
    if (hasAnyUsers()) throw AccessDeniedException("The system is already set up.");
    UserDetails d;
    d.username = username;
    d.fullName = fullName;
    d.referenceNumber = "";
    validateUsername(username);
    validatePassword(password);
    d.salt = hasher_.generateSalt();
    d.passwordHash = hasher_.hash(password, d.salt);
    return users_.save(*UserFactory::create(Role::Administrator, d));
}

int AuthService::createUser(const User& actor, Role role, UserDetails details, const std::string& password) {
    requirePermission(actor, Permission::ManageUsers, "manage user accounts");
    validateUsername(details.username);
    validatePassword(password);
    if (details.fullName.empty()) throw ValidationException("The full name is required.");
    if (users_.findByUsername(details.username)) throw ValidationException("That username is already taken.");
    if (role == Role::HeadOfDepartment) {
        for (const std::unique_ptr<User>& u : users_.findAll())
            if (u->getRole() == Role::HeadOfDepartment && u->getDepartmentId() == details.departmentId)
                throw ValidationException("That department already has a head of department.");
    }
    details.salt = hasher_.generateSalt();
    details.passwordHash = hasher_.hash(password, details.salt);
    return users_.save(*UserFactory::create(role, details));
}

std::vector<std::unique_ptr<User>> AuthService::listUsers(const User& actor) {
    requirePermission(actor, Permission::ManageUsers, "list user accounts");
    return users_.findAll();
}

}  // namespace lms
