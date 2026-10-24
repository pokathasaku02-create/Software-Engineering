#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../data/Repositories.h"
#include "../domain/UserFactory.h"
#include "PasswordHasher.h"

namespace lms {

class AuthService {
public:
    AuthService(IUserRepository& users, const PasswordHasher& hasher);

    // Throws AuthenticationException with one generic message for every failure.
    const User* login(const std::string& username, const std::string& password);
    void logout();
    const User* currentUser() const { return current_.get(); }

    bool hasAnyUsers();
    // First-run set-up: only allowed while the user table is empty.
    int createInitialAdministrator(const std::string& username, const std::string& fullName, const std::string& password);
    // Administrator only. Hashes the password with a fresh salt; builds the user through UserFactory.
    int createUser(const User& actor, Role role, UserDetails details, const std::string& password);
    std::vector<std::unique_ptr<User>> listUsers(const User& actor);

    static void validatePassword(const std::string& password);
    static void validateUsername(const std::string& username);

private:
    IUserRepository& users_;
    const PasswordHasher& hasher_;
    std::unique_ptr<User> current_;
    std::string dummySalt_, dummyHash_;   // used to keep login timing similar for unknown users
};

}  // namespace lms
