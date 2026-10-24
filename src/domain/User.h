// Abstract User and its four role subclasses (polymorphism: each role decides what it may do).
#pragma once
#include <optional>
#include <string>
#include <utility>

#include "Types.h"

namespace lms {

class User {
public:
    User(int id, std::string username, std::string passwordHash, std::string salt, std::string fullName, bool active)
        : userId_(id), username_(std::move(username)), passwordHash_(std::move(passwordHash)),
          salt_(std::move(salt)), fullName_(std::move(fullName)), active_(active) {}
    virtual ~User() = default;

    int getId() const { return userId_; }
    const std::string& getUsername() const { return username_; }
    const std::string& getPasswordHash() const { return passwordHash_; }
    const std::string& getSalt() const { return salt_; }
    const std::string& getFullName() const { return fullName_; }
    bool isActive() const { return active_; }
    void setId(int id) { userId_ = id; }
    void setActive(bool active) { active_ = active; }

    virtual Role getRole() const = 0;
    virtual bool canPerform(Permission p) const = 0;
    // Student number or staff number; empty for administrators.
    virtual std::string getReferenceNumber() const { return {}; }
    // Department of a lecturer or Head of Department; empty otherwise.
    virtual std::optional<int> getDepartmentId() const { return std::nullopt; }

private:
    int userId_;
    std::string username_, passwordHash_, salt_, fullName_;
    bool active_;
};

class Student : public User {
public:
    Student(int id, std::string username, std::string hash, std::string salt, std::string fullName, bool active,
            std::string studentNumber)
        : User(id, std::move(username), std::move(hash), std::move(salt), std::move(fullName), active),
          studentNumber_(std::move(studentNumber)) {}
    Role getRole() const override { return Role::Student; }
    bool canPerform(Permission p) const override;
    std::string getReferenceNumber() const override { return studentNumber_; }
private:
    std::string studentNumber_;
};

class Lecturer : public User {
public:
    Lecturer(int id, std::string username, std::string hash, std::string salt, std::string fullName, bool active,
             std::string staffNumber, int departmentId)
        : User(id, std::move(username), std::move(hash), std::move(salt), std::move(fullName), active),
          staffNumber_(std::move(staffNumber)), departmentId_(departmentId) {}
    Role getRole() const override { return Role::Lecturer; }
    bool canPerform(Permission p) const override;
    std::string getReferenceNumber() const override { return staffNumber_; }
    std::optional<int> getDepartmentId() const override { return departmentId_; }
private:
    std::string staffNumber_;
    int departmentId_;
};

class HeadOfDepartment : public User {
public:
    HeadOfDepartment(int id, std::string username, std::string hash, std::string salt, std::string fullName,
                     bool active, std::string staffNumber, int departmentId)
        : User(id, std::move(username), std::move(hash), std::move(salt), std::move(fullName), active),
          staffNumber_(std::move(staffNumber)), departmentId_(departmentId) {}
    Role getRole() const override { return Role::HeadOfDepartment; }
    bool canPerform(Permission p) const override;
    std::string getReferenceNumber() const override { return staffNumber_; }
    std::optional<int> getDepartmentId() const override { return departmentId_; }
private:
    std::string staffNumber_;
    int departmentId_;
};

class Administrator : public User {
public:
    using User::User;
    Role getRole() const override { return Role::Administrator; }
    bool canPerform(Permission p) const override;
};

}  // namespace lms
