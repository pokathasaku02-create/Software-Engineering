// Composition root: builds the object graph (the only place that knows every concrete class)
// and starts the log-in menu.
#include <iostream>
#include <string>

#include "data/Database.h"
#include "data/SqliteRepositories.h"
#include "domain/IClock.h"
#include "domain/LatePenaltyPolicy.h"
#include "service/AssignmentService.h"
#include "service/AuditService.h"
#include "service/AuthService.h"
#include "service/CourseService.h"
#include "service/ModerationService.h"
#include "service/PasswordHasher.h"
#include "service/SubmissionService.h"
#include "ui/Console.h"
#include "ui/Menu.h"

int main(int argc, char** argv) {
    using namespace lms;
    const std::string dbPath = argc > 1 ? argv[1] : "lms.db";
    try {
        Database db(dbPath);
        db.initialiseSchema();

        SqliteUserRepository users(db);
        SqliteCourseRepository courses(db);
        SqliteAssignmentRepository assignments(db);
        SqliteSubmissionRepository submissions(db);
        SqliteAuditRepository audits(db);

        SystemClock clock;
        PasswordHasher hasher;
        TenPercentPerDayPolicy policy;   // swap for another LatePenaltyPolicy to change the rule

        AuthService auth(users, hasher);
        AuditService audit(audits, clock);
        CourseService courseSvc(courses, users, clock);
        AssignmentService assignmentSvc(assignments, courses, clock);
        SubmissionService submissionSvc(assignments, submissions, courses, audit, policy, db, clock);
        ModerationService moderationSvc(courses, submissions, assignments, audit, db);

        StdConsole console;
        LoginMenu login(console, Services{auth, courseSvc, assignmentSvc, submissionSvc, moderationSvc, audit});
        login.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
