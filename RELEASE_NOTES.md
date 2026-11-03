# Release Notes — Moderated LMS

## v1.0 — Initial release

Release date: 7th November 2026
Tag: v1.0
Platform: Ubuntu Linux 22.04 or later; single school computer, fully offline.

### Summary
The first release of the Moderated Learning Management System: a console C++17 application with SQLite persistence, role-based access for four user types, late-penalty and extension rules, moderated mark release, and a full audit log of every mark change.

### Features
- Login and roles — students, lecturers, Heads of Department and administrators; every action is checked against the user's role.
- Administrator — manage users, departments and courses; assign lecturers; enrol students.
- Lecturer — create assignments (deadline, maximum mark); grant individual extensions; mark submissions with feedback; change a mark with a required reason.
- Head of Department — review all marks in their department, moderate marks, and release approved marks to students.
- Student — view courses and assignments; submit and resubmit before the deadline; view released marks and written feedback.
- Late penalty — 10% of the maximum mark per day late, up to 3 days, then zero (Strategy pattern).
- Submission life cycle — Submitted, Late, Marked, Moderated, Released (state machine).
- Audit log — every mark change records who, when, old value, new value and reason; the audit table forbids UPDATE and DELETE.
- Security — Argon2id salted password hashes (libsodium), parameterised SQL throughout, input validation, and exceptions caught so bad input never crashes the program.
- Persistence — SQLite database accessed only through a repository/data-access layer.

### Verification
- 164 automated GoogleTest tests covering unit, integration, system and acceptance levels.
- CI (GitHub Actions) builds the project and runs the tests on every push, failing if business-logic coverage drops below 70%.
- Static analysis (cppcheck / clang-tidy) results and code metrics in docs/quality/.
- Test plan, results, coverage report and defect log in docs/testing/.

### Known limitations
- No account lock-out after repeated failed logins, and no password-change screen.
- Console interface only; some Windows consoles echo passwords while typing.
- Extensions are shown only when granting them; they are not shown on the student's assignment list in v1.0 — this is addressed by change request CR1 in v1.1.
- Late-penalty fairness and date/time handling assume one school clock with no daylight-saving changes.

### Build and run
See README.md section 1 for the build guide and docs/deployment/INSTALLATION_GUIDE.md for installation, permissions and backups.

## v1.1 — Planned (CR1)

Change request CR1: "A lecturer should be able to extend the deadline for a whole class, not just an individual student, and extensions should be shown on the student's assignment list."

Planned for v1.1:
- Class-wide deadline extensions for a lecturer's course.
- Extensions displayed on the student's assignment list.
- Updated design documents and regression tests.

(Release notes for v1.1 will be added when the change is implemented.)
