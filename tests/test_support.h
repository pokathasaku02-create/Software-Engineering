// Shared test fixtures: an in-memory "school" wired exactly like main.cpp, plus a scripted console.
#pragma once
#include <gtest/gtest.h>

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "data/Database.h"
#include "data/SqliteRepositories.h"
#include "domain/Exceptions.h"
#include "domain/IClock.h"
#include "domain/LatePenaltyPolicy.h"
#include "domain/UserFactory.h"
#include "service/AssignmentService.h"
#include "service/AuditService.h"
#include "service/AuthService.h"
#include "service/CourseService.h"
#include "service/ModerationService.h"
#include "service/PasswordHasher.h"
#include "service/SubmissionService.h"
#include "ui/Console.h"
#include "ui/Menu.h"

namespace lms::test {

inline DateTime T(const char* text) { return parseDateTime(text); }

// A complete application stack on a private in-memory database. Nothing touches lms.db.
struct World {
    Database db{":memory:"};
    SqliteUserRepository userRepo{db};
    SqliteCourseRepository courseRepo{db};
    SqliteAssignmentRepository assignmentRepo{db};
    SqliteSubmissionRepository submissionRepo{db};
    SqliteAuditRepository auditRepo{db};
    FixedClock clock{T("2026-10-01 09:00")};
    PasswordHasher hasher{true};                 // cheap Argon2 settings: tests stay fast
    TenPercentPerDayPolicy policy;
    AuthService auth{userRepo, hasher};
    AuditService audit{auditRepo, clock};
    CourseService courseSvc{courseRepo, userRepo, clock};
    AssignmentService assignmentSvc{assignmentRepo, courseRepo, clock};
    SubmissionService submissionSvc{assignmentRepo, submissionRepo, courseRepo, audit, policy, db, clock};
    ModerationService moderationSvc{courseRepo, submissionRepo, assignmentRepo, audit, db};

    std::unique_ptr<User> admin, lecturer, hod, student1, student2;
    int dept = 0, course = 0, assignmentId = 0;

    World() { db.initialiseSchema(); }

    Services services() { return Services{auth, courseSvc, assignmentSvc, submissionSvc, moderationSvc, audit}; }

    std::unique_ptr<User> addUser(Role role, const std::string& username, std::optional<int> departmentId = std::nullopt,
                                  const std::string& password = "password1") {
        if (!auth.hasAnyUsers()) {
            auth.createInitialAdministrator("admin", "Administrator", "adminpass1");
            admin = userRepo.findByUsername("admin");
        }
        UserDetails d;
        d.username = username;
        d.fullName = "Name of " + username;
        d.referenceNumber = role == Role::Administrator ? "" : "REF-" + username;
        d.departmentId = departmentId;
        const int id = auth.createUser(*admin, role, d, password);
        return userRepo.findById(id);
    }

    // Science department, course CS101 taught by lecturer, two enrolled students,
    // one assignment (max 100) due 2026-10-10 12:00. Clock: 2026-10-01 09:00.
    void setUpStandard() {
        admin = addUser(Role::Administrator, "admin2");
        dept = courseSvc.createDepartment(*admin, "Science");
        lecturer = addUser(Role::Lecturer, "lecturer1", dept);
        hod = addUser(Role::HeadOfDepartment, "hod1", dept);
        student1 = addUser(Role::Student, "student1");
        student2 = addUser(Role::Student, "student2");
        course = courseSvc.createCourse(*admin, CourseData{"CS101", "Introduction", dept});
        courseSvc.assignLecturer(*admin, course, lecturer->getId());
        courseSvc.enrolStudent(*admin, course, student1->getId());
        courseSvc.enrolStudent(*admin, course, student2->getId());
        AssignmentData a;
        a.courseId = course; a.title = "Assignment 1"; a.description = "Essay";
        a.deadline = T("2026-10-10 12:00"); a.maxMark = 100;
        assignmentId = assignmentSvc.createAssignment(*lecturer, a);
    }
};

class LmsTest : public ::testing::Test {
protected:
    void SetUp() override { w.setUpStandard(); }
    void at(const char* when) { w.clock.set(T(when)); }
    Receipt submitAt(const char* when, User& student, const std::string& work = "my work") {
        at(when);
        return w.submissionSvc.submit(student, w.assignmentId, work);
    }
    std::optional<Submission> load(int id) { return w.submissionRepo.findById(id); }
    World w;
};

// Console driven by a fixed list of input lines; records everything printed or prompted.
class ScriptedConsole : public Console {
public:
    explicit ScriptedConsole(std::vector<std::string> lines) : in_(lines.begin(), lines.end()) {}
    void println(const std::string& text) override { out_.push_back(text); }
    std::string readLine(const std::string& prompt) override { return next(prompt); }
    std::string readSecret(const std::string& prompt) override { return next(prompt); }
    bool printed(const std::string& needle) const { return transcript().find(needle) != std::string::npos; }
    std::string transcript() const {
        std::string all;
        for (const std::string& s : out_) all += s + "\n";
        return all;
    }
private:
    std::string next(const std::string& prompt) {
        out_.push_back(prompt);
        if (in_.empty()) throw InputClosedException("script finished");
        std::string line = in_.front();
        in_.pop_front();
        return line;
    }
    std::deque<std::string> in_;
    std::vector<std::string> out_;
};

}  // namespace lms::test
