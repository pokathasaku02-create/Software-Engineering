#include "Menu.h"

#include <climits>
#include <memory>

#include "../domain/Exceptions.h"
#include "../domain/UserFactory.h"

namespace lms {
namespace {

std::string markText(const std::optional<int>& m) { return m ? std::to_string(*m) : "-"; }

void printAssignment(Console& io, const Assignment& a) {
    io.println("  #" + std::to_string(a.getId()) + "  " + a.getTitle() + "  (course " + std::to_string(a.getCourseId()) +
               ", deadline " + formatDateTime(a.getDeadline()) + ", max mark " + std::to_string(a.getMaxMark()) + ")");
}

void printSubmission(Console& io, const Submission& s) {
    io.println("  #" + std::to_string(s.getId()) + "  assignment " + std::to_string(s.getAssignmentId()) + ", student " +
               std::to_string(s.getStudentId()) + ", " + toString(s.getStatus()) + ", submitted " +
               formatDateTime(s.getSubmittedAt()) + ", raw " + markText(s.getRawMark()) + ", final " + markText(s.getFinalMark()));
}

void printAudit(Console& io, const std::vector<AuditEntry>& history) {
    if (history.empty()) io.println("  (no mark changes recorded)");
    for (const AuditEntry& e : history)
        io.println("  " + formatDateTime(e.getChangedAt()) + "  by user " + std::to_string(e.getChangedBy()) + "  " +
                   markText(e.getOldValue()) + " -> " + std::to_string(e.getNewValue()) + "  reason: " + e.getReason());
}

}  // namespace

void Menu::run() {
    for (;;) {
        const std::vector<std::string> opts = options();
        say("");
        say("=== " + title() + " (" + user_.getFullName() + ") ===");
        for (std::size_t i = 0; i < opts.size(); ++i) say("  " + std::to_string(i + 1) + ". " + opts[i]);
        say("  0. Log out");
        try {
            const int choice = InputValidator::integer(ask("Choice: "), "The choice", 0, static_cast<int>(opts.size()));
            if (choice == 0) return;
            handle(choice);
        } catch (const InputClosedException&) {
            throw;
        } catch (const DatabaseException& e) {
            say("The database could not complete that request: " + std::string(e.what()));
        } catch (const LmsException& e) {
            say("Error: " + std::string(e.what()));
        } catch (const std::exception& e) {
            say(std::string("An unexpected error occurred and the action was cancelled: ") + e.what());
        }
    }
}

void Menu::printCourses() {
    const std::vector<Course> courses = svc_.courses.coursesFor(user_);
    if (courses.empty()) say("  (no courses)");
    for (const Course& c : courses)
        say("  #" + std::to_string(c.getId()) + "  " + c.getCode() + "  " + c.getTitle() + "  (department " +
            std::to_string(c.getDepartmentId()) + ", lecturer " +
            (c.getLecturerId() ? std::to_string(*c.getLecturerId()) : std::string("none")) + ")");
}

// ------------------------------------------------------------------ student

void StudentMenu::handle(int choice) {
    switch (choice) {
        case 1: printCourses(); break;
        case 2: {
            const std::vector<Assignment> list = svc_.assignments.assignmentsFor(user_);
            if (list.empty()) say("  (no assignments)");
            for (const Assignment& a : list) {
                printAssignment(io_, a);
                if (std::optional<Submission> s = svc_.submissions.statusFor(user_, a.getId()))
                    say("      your submission: " + toString(s->getStatus()) + " (submitted " + formatDateTime(s->getSubmittedAt()) + ")");
            }
            break;
        }
        case 3: {
            const int id = InputValidator::integer(ask("Assignment number: "), "The assignment number", 1, INT_MAX);
            const std::string work = InputValidator::text(ask("Your work (text or file name): "), "The work", 1, 20000);
            const Receipt r = svc_.submissions.submit(user_, id, work);
            say(std::string(r.resubmission ? "Resubmitted" : "Submitted") + " at " + formatDateTime(r.submittedAt) +
                ". Status: " + toString(r.status) + (r.status == SubmissionStatus::Late ? " (late work loses marks)." : "."));
            break;
        }
        case 4: {
            const std::vector<Submission> list = svc_.submissions.releasedMarksFor(user_);
            if (list.empty()) say("  (no marks have been released yet)");
            for (const Submission& s : list) {
                const Assignment a = svc_.assignments.getAssignment(s.getAssignmentId());
                say("  " + a.getTitle() + ": " + markText(s.getFinalMark()) + " / " + std::to_string(a.getMaxMark()));
                say("      feedback: " + s.getFeedback());
            }
            break;
        }
    }
}

// ------------------------------------------------------------------ lecturer

void LecturerMenu::handle(int choice) {
    switch (choice) {
        case 1: printCourses(); break;
        case 2: {
            AssignmentData d;
            d.courseId = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            d.title = InputValidator::text(ask("Title: "), "The title", 1, 100);
            d.description = InputValidator::text(ask("Description (may be empty): "), "The description", 0, 2000);
            d.deadline = InputValidator::dateTime(ask("Deadline (YYYY-MM-DD HH:MM): "));
            d.maxMark = InputValidator::integer(ask("Maximum mark: "), "The maximum mark", 1, 1000);
            say("Assignment #" + std::to_string(svc_.assignments.createAssignment(user_, d)) + " created.");
            break;
        }
        case 3: {
            const int c = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            for (const Assignment& a : svc_.assignments.assignmentsForCourse(user_, c)) printAssignment(io_, a);
            break;
        }
        case 4: {
            ExtensionData e;
            e.assignmentId = InputValidator::integer(ask("Assignment number: "), "The assignment number", 1, INT_MAX);
            e.studentId = InputValidator::integer(ask("Student user number: "), "The student number", 1, INT_MAX);
            e.newDeadline = InputValidator::dateTime(ask("New deadline (YYYY-MM-DD HH:MM): "));
            e.reason = InputValidator::text(ask("Reason: "), "The reason", 1, 500);
            svc_.assignments.grantExtension(user_, e);
            say("Extension granted.");
            break;
        }
        case 5: {
            const int c = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            const std::vector<Submission> list = svc_.submissions.submissionsToMark(user_, c);
            if (list.empty()) say("  (nothing to mark)");
            for (const Submission& s : list) printSubmission(io_, s);
            break;
        }
        case 6: {
            const int id = InputValidator::integer(ask("Submission number: "), "The submission number", 1, INT_MAX);
            const int raw = InputValidator::integer(ask("Mark (before any late penalty): "), "The mark", 0, 1000);
            const std::string fb = InputValidator::text(ask("Written feedback: "), "The feedback", 1, 2000);
            const std::string reason = InputValidator::text(ask("Reason (required only when changing an existing mark): "), "The reason", 0, 500);
            svc_.submissions.markSubmission(user_, id, raw, fb, reason);
            say("Mark saved and recorded in the audit log. The student cannot see it until it is moderated and released.");
            break;
        }
    }
}

// ------------------------------------------------------------------ head of department

void HoDMenu::handle(int choice) {
    switch (choice) {
        case 1: printCourses(); break;
        case 2: {
            const int c = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            const std::vector<Submission> list = svc_.moderation.marksForReview(user_, c);
            if (list.empty()) say("  (no marks waiting for review)");
            for (const Submission& s : list) printSubmission(io_, s);
            break;
        }
        case 3: {
            const int id = InputValidator::integer(ask("Submission number: "), "The submission number", 1, INT_MAX);
            const int mark = InputValidator::integer(ask("New mark: "), "The mark", 0, 1000);
            const std::string reason = InputValidator::text(ask("Reason: "), "The reason", 1, 500);
            svc_.moderation.adjustMark(user_, id, mark, reason);
            say("Mark adjusted and recorded in the audit log.");
            break;
        }
        case 4: {
            const int id = InputValidator::integer(ask("Submission number: "), "The submission number", 1, INT_MAX);
            svc_.moderation.approve(user_, id);
            say("Approved.");
            break;
        }
        case 5: {
            const int c = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            int n = 0;
            for (const Submission& s : svc_.moderation.marksForReview(user_, c)) { svc_.moderation.approve(user_, s.getId()); ++n; }
            say(std::to_string(n) + " mark(s) approved.");
            break;
        }
        case 6: {
            const int c = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            say(std::to_string(svc_.moderation.releaseApproved(user_, c)) + " mark(s) released to students.");
            break;
        }
        case 7: {
            const int id = InputValidator::integer(ask("Submission number: "), "The submission number", 1, INT_MAX);
            printAudit(io_, svc_.audit.historyOf(user_, id));
            break;
        }
    }
}

// ------------------------------------------------------------------ administrator

void AdminMenu::handle(int choice) {
    switch (choice) {
        case 1: {
            const std::string r = InputValidator::text(ask("Role (student, lecturer, hod, admin): "), "The role", 3, 10);
            Role role;
            if (r == "student") role = Role::Student;
            else if (r == "lecturer") role = Role::Lecturer;
            else if (r == "hod") role = Role::HeadOfDepartment;
            else if (r == "admin") role = Role::Administrator;
            else throw ValidationException("The role must be student, lecturer, hod or admin.");
            UserDetails d;
            d.username = InputValidator::username(ask("Username: "));
            d.fullName = InputValidator::text(ask("Full name: "), "The full name", 1, 100);
            if (role != Role::Administrator)
                d.referenceNumber = InputValidator::text(ask(role == Role::Student ? "Student number: " : "Staff number: "), "The number", 1, 30);
            if (role == Role::Lecturer || role == Role::HeadOfDepartment) {
                const std::vector<Department> deps = svc_.courses.departments();
                for (const Department& dep : deps) say("  department #" + std::to_string(dep.getId()) + "  " + dep.getName());
                d.departmentId = InputValidator::integer(ask("Department number: "), "The department number", 1, INT_MAX);
                bool exists = false;
                for (const Department& dep : deps) exists = exists || dep.getId() == *d.departmentId;
                if (!exists) throw ValidationException("No such department.");
            }
            const std::string pw = InputValidator::password(io_.readSecret("Initial password (min 8 characters): "));
            say("User #" + std::to_string(svc_.auth.createUser(user_, role, d, pw)) + " created.");
            break;
        }
        case 2:
            for (const std::unique_ptr<User>& u : svc_.auth.listUsers(user_))
                say("  #" + std::to_string(u->getId()) + "  " + u->getUsername() + "  " + u->getFullName() + "  " + toString(u->getRole()) +
                    (u->isActive() ? "" : "  [disabled]"));
            break;
        case 3: {
            const std::string n = InputValidator::text(ask("Department name: "), "The name", 1, 100);
            say("Department #" + std::to_string(svc_.courses.createDepartment(user_, n)) + " created.");
            break;
        }
        case 4: {
            for (const Department& dep : svc_.courses.departments()) say("  department #" + std::to_string(dep.getId()) + "  " + dep.getName());
            CourseData c;
            c.code = InputValidator::text(ask("Course code: "), "The code", 1, 20);
            c.title = InputValidator::text(ask("Course title: "), "The title", 1, 100);
            c.departmentId = InputValidator::integer(ask("Department number: "), "The department number", 1, INT_MAX);
            say("Course #" + std::to_string(svc_.courses.createCourse(user_, c)) + " created.");
            break;
        }
        case 5: {
            const int c = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            const int l = InputValidator::integer(ask("Lecturer user number: "), "The lecturer number", 1, INT_MAX);
            svc_.courses.assignLecturer(user_, c, l);
            say("Lecturer assigned.");
            break;
        }
        case 6: {
            const int c = InputValidator::integer(ask("Course number: "), "The course number", 1, INT_MAX);
            const int s = InputValidator::integer(ask("Student user number: "), "The student number", 1, INT_MAX);
            svc_.courses.enrolStudent(user_, c, s);
            say("Student enrolled.");
            break;
        }
        case 7: printCourses(); break;
        case 8: {
            const int id = InputValidator::integer(ask("Submission number: "), "The submission number", 1, INT_MAX);
            printAudit(io_, svc_.audit.historyOf(user_, id));
            break;
        }
    }
}

// ------------------------------------------------------------------ login

void LoginMenu::firstRunSetup() {
    io_.println("First run: no accounts exist yet. Create the administrator account.");
    for (;;) {
        try {
            const std::string u = InputValidator::username(io_.readLine("Administrator username: "));
            const std::string n = InputValidator::text(io_.readLine("Full name: "), "The full name", 1, 100);
            const std::string p = InputValidator::password(io_.readSecret("Password (min 8 characters): "));
            svc_.auth.createInitialAdministrator(u, n, p);
            io_.println("Administrator account created. Please log in.");
            return;
        } catch (const InputClosedException&) {
            throw;
        } catch (const LmsException& e) {
            io_.println(std::string("Error: ") + e.what());
        }
    }
}

void LoginMenu::sessionFor(const User& user) {
    std::unique_ptr<Menu> menu;
    switch (user.getRole()) {
        case Role::Student: menu = std::make_unique<StudentMenu>(io_, svc_, user); break;
        case Role::Lecturer: menu = std::make_unique<LecturerMenu>(io_, svc_, user); break;
        case Role::HeadOfDepartment: menu = std::make_unique<HoDMenu>(io_, svc_, user); break;
        case Role::Administrator: menu = std::make_unique<AdminMenu>(io_, svc_, user); break;
    }
    menu->run();
}

void LoginMenu::run() {
    try {
        if (!svc_.auth.hasAnyUsers()) firstRunSetup();
        for (;;) {
            io_.println("");
            io_.println("=== Moderated LMS: log in (leave the username empty to exit) ===");
            try {
                const std::string u = io_.readLine("Username: ");
                if (u.empty()) return;
                const std::string p = io_.readSecret("Password: ");
                const User* user = svc_.auth.login(u, p);
                sessionFor(*user);
                svc_.auth.logout();
            } catch (const InputClosedException&) {
                throw;
            } catch (const LmsException& e) {
                io_.println(std::string("Error: ") + e.what());
            } catch (const std::exception& e) {
                io_.println(std::string("An unexpected error occurred: ") + e.what());
            }
        }
    } catch (const InputClosedException&) {
        io_.println("");
        io_.println("Input closed. Goodbye.");
    }
    svc_.auth.logout();
}

}  // namespace lms
