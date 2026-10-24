// Role menus. They only read input, call services and print results: no business logic.
#pragma once
#include <string>
#include <vector>

#include "../domain/User.h"
#include "../service/AssignmentService.h"
#include "../service/AuditService.h"
#include "../service/AuthService.h"
#include "../service/CourseService.h"
#include "../service/ModerationService.h"
#include "../service/SubmissionService.h"
#include "Console.h"
#include "InputValidator.h"

namespace lms {

struct Services {
    AuthService& auth;
    CourseService& courses;
    AssignmentService& assignments;
    SubmissionService& submissions;
    ModerationService& moderation;
    AuditService& audit;
};

class Menu {
public:
    Menu(Console& console, Services services, const User& user) : io_(console), svc_(services), user_(user) {}
    virtual ~Menu() = default;
    // Shows the menu until the user logs out. Errors are reported and never end the program.
    void run();

protected:
    virtual std::string title() const = 0;
    virtual std::vector<std::string> options() const = 0;      // option n is options()[n-1]
    virtual void handle(int choice) = 0;                        // called for 1..options().size()

    void printCourses();
    std::string ask(const std::string& prompt) { return io_.readLine(prompt); }
    void say(const std::string& text) { io_.println(text); }

    Console& io_;
    Services svc_;
    const User& user_;
};

class StudentMenu : public Menu {
public:
    using Menu::Menu;
protected:
    std::string title() const override { return "Student menu"; }
    std::vector<std::string> options() const override {
        return {"View my courses", "View my assignments", "Submit or resubmit work", "View released marks and feedback"};
    }
    void handle(int choice) override;
};

class LecturerMenu : public Menu {
public:
    using Menu::Menu;
protected:
    std::string title() const override { return "Lecturer menu"; }
    std::vector<std::string> options() const override {
        return {"View my courses", "Create an assignment", "List assignments of a course",
                "Grant an extension to a student", "List submissions to mark", "Mark or re-mark a submission"};
    }
    void handle(int choice) override;
};

class HoDMenu : public Menu {
public:
    using Menu::Menu;
protected:
    std::string title() const override { return "Head of Department menu"; }
    std::vector<std::string> options() const override {
        return {"View courses in my department", "Review marks of a course", "Adjust a mark", "Approve a mark",
                "Approve all marks of a course", "Release approved marks of a course", "View audit history of a submission"};
    }
    void handle(int choice) override;
};

class AdminMenu : public Menu {
public:
    using Menu::Menu;
protected:
    std::string title() const override { return "Administrator menu"; }
    std::vector<std::string> options() const override {
        return {"Create a user account", "List user accounts", "Create a department", "Create a course",
                "Assign a lecturer to a course", "Enrol a student in a course", "List courses",
                "View audit history of a submission"};
    }
    void handle(int choice) override;
};

// Log-in screen and role dispatch (the program's main loop).
class LoginMenu {
public:
    LoginMenu(Console& console, Services services) : io_(console), svc_(services) {}
    void run();                       // returns when input is closed or the user chooses Exit
private:
    void firstRunSetup();
    void sessionFor(const User& user);
    Console& io_;
    Services svc_;
};

}  // namespace lms
