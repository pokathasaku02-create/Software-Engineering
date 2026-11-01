# Test plan: Moderated LMS

Phase 5 deliverable (verification and validation). Version 1.0.

## 1. Purpose and scope
This plan shows how each artefact of the V-model is verified, which techniques are used, and when testing starts and stops.
In scope: every Must requirement of the SRS, the business rules from the case study (late penalty, extensions, resubmission,
moderation, release, audit), role-based access, persistence and security. Out of scope (as in the case study): web or mobile
interfaces, payments, automated grading, plagiarism detection.

## 2. V-model: what each level verifies

| V-model level | Verifies (artefact) | Test basis | Technique | Test code |
|---|---|---|---|---|
| Unit | Detailed design: domain classes, `LatePenaltyPolicy`, `Submission` state machine, `UserFactory`, `PasswordHasher`, `InputValidator`, date helpers | Class diagrams (figures 3a, 3b), state machine (3c) | Black box with equivalence partitioning and boundary value analysis; transition-matrix testing for the state machine | `test_domain.cpp`, parts of `test_auth_course.cpp`, `test_ui_system.cpp` |
| Integration | Architecture: services working with the real SQLite repositories; transactions; database constraints | Component diagram, class diagram part 2, ERD (3g), sequence diagrams (3e, 3f) | Interface and constraint testing against an in-memory SQLite database; fault injection (dropped table) | `test_repositories.cpp`, `test_auth_course.cpp`, `test_submission_moderation.cpp` |
| System | The whole program through its menus against the SRS and use-case specifications | Use cases: Log in, Submit assignment, Mark submission, Moderate and release marks, Administer; main, alternative and exception flows | Scripted console sessions (`ScriptedConsole` feeds the same input a user would type) | `test_ui_system.cpp` (`SystemTest`) |
| Acceptance | Stakeholder needs and measurable non-functional requirements | Stakeholder statements in the case study; NFRs | One test per statement; timing measurements | `test_ui_system.cpp` (`AcceptanceTest`, `Performance`) |

Number of automated tests at release 1.0:

| Level | Tests | Suites |
|---|---|---|
| Unit (domain, helpers) | 41 | DateTime (6), Enums (2), Clock (2), TenPercentPolicy (7), NoPenaltyPolicy (1), Deadlines (2), Entities (1), SubmissionLifecycle (7), Permissions (1), Users (1), UserFactory (2), PasswordHasher (6), InputValidator (3) |
| Integration (repositories, services with real SQLite) | 94 | RepositoryTest (21), DatabaseTest (4), AuthTest (15), AuthSetup (1), CourseServiceTest (6), AssignmentServiceTest (11), SubmissionTest (27), ModerationTest (9) |
| System (use-case flows through the menus) | 21 | SystemFirstRun (1), SystemTest (20) |
| Acceptance (stakeholder statements and NFRs) | 8 | AcceptanceTest (6), Performance (2) |
| **Total** | **164** | |

## 3. Entry and exit criteria

| Level | Entry criteria | Exit criteria |
|---|---|---|
| Unit | Code for the unit compiles with no warnings (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion`); detailed design (Gate 2) approved | All unit tests pass; every state-machine transition (legal and illegal) has a test; every boundary in section 4 has a test |
| Integration | All unit tests pass; repositories and services build into `lms_core` | All integration tests pass; every table constraint has a test; mark change and audit entry proven atomic |
| System | Integration tests pass; program builds and starts | All system tests pass, including every exception flow; no open defect of severity High or Critical |
| Acceptance | System tests pass | Every stakeholder statement has a passing test; NFR measurements are within limits (login under 2 s with 800 users stored) |
| Whole phase (Gate 3) | | 100% of automated tests pass in CI; line coverage of the business-logic layer (`src/domain` + `src/service`) at least 70% (target met: see COVERAGE_REPORT.md); every defect in the log is closed or accepted with a reason; the requirements traceability matrix shows no untested Must requirement |

## 4. Test design techniques

### 4.1 Submission timing (deadline D = 2026-10-10 12:00:00; extension E = 2026-10-13 12:00:00)
Equivalence classes: before the deadline, on the deadline, after the deadline, after the grace period of three days.
Boundary values tested:

| Submitted at | Expected status | Expected final mark for raw 80 (max 100) | Test |
|---|---|---|---|
| D - 1 s | SUBMITTED | 80 | `SubmissionTest.OnTimeBoundaryValues` |
| D (exactly) | SUBMITTED | 80 | same |
| D + 1 s | LATE | 70 (1 day late) | `OnTimeBoundaryValues`, `LatePenaltyBoundaryValues` |
| D + 1 day | LATE | 70 | `LatePenaltyBoundaryValues` |
| D + 1 day + 1 s | LATE | 60 (2 days) | same |
| D + 3 days | LATE | 50 | same |
| D + 3 days + 1 s | LATE | 0 (beyond grace) | same |
| With extension: E, E + 1 s | SUBMITTED, LATE | measured from E | `ExtensionBoundaryValues`, `PenaltyIsMeasuredFromTheExtendedDeadline` |

### 4.2 Late penalty (`LatePenaltyPolicy`)
Partitions of `daysLate`: below 0, 0, 1, 2, 3, 4, very large. Partitions of marks: raw mark smaller than the deduction (result clamped to 0), maximum marks of 7, 15, 50, 100 and 1000 (rounding and scaling).
Tests: `TenPercentPolicy.*`, `NoPenaltyPolicy.*`, `Deadlines.DaysLateBoundaryValues`.

### 4.3 Input and range boundaries

| Field | Valid range | Boundary values tested |
|---|---|---|
| Mark | 0 to maxMark | -1, 0, 100, 101 |
| Maximum mark | 1 to 1000 | 0, 1, 1000, 1001 |
| Title | 1 to 100 chars | 0, 100, 101 |
| Description | 0 to 2000 chars | 2000, 2001 |
| Feedback | 1 to 2000 chars | 0, 2000, 2001 |
| Reason | 1 to 500 chars | 0, 500, 501 |
| Submitted work | 1 to 20000 chars | 0, 1, 20000, 20001 |
| Password | 8 to 128 chars | 7, 8, 128, 129 |
| Username | 3 to 32 chars, letters, digits, `_ . -` | 2, 3, 32, 33; space, quote, semicolon, non-ASCII |
| Dates | real calendar dates | 30 Feb, 31 Apr, 29 Feb (leap and non-leap years), 24:00, minute 60, second 60 |
| Menu choice / numbers | whole number in range | empty, text, 1.5, 12abc, overflow, hex |

### 4.4 State machine
`SubmissionLifecycle.*` tries every action (resubmit, mark, adjust, approve, release) in every one of the five states and checks that exactly the transitions in figure 3c succeed and all others throw `IllegalStateException`.

### 4.5 Security tests
Plain-text password never stored (scan of the users table), salts unique, wrong password, unknown user and disabled account give one identical message, SQL injection strings in login and in stored text, audit table cannot be updated or deleted, role checks for every service method (student, lecturer, HoD, administrator, HoD of another department, disabled account).

### 4.6 Fault injection
`SubmissionTest.MarkAndAuditEntryAreAtomic` drops the audit table and proves the mark change is rolled back. `SystemTest.SubmitAssignmentExceptionFlowDatabaseFailureDoesNotCrash` drops the submission table and proves the menu reports the error and keeps running.

## 5. Test environment and tools
- Language and framework: C++17, GoogleTest (via CMake `FetchContent` or a system package), run with CTest.
- Database: SQLite in memory (`":memory:"`) for every test. Tests never touch `lms.db`.
- Time: `FixedClock` makes every deadline test deterministic.
- Password hashing: reduced Argon2 cost in tests (`PasswordHasher(true)`), full production cost in the performance test.
- Coverage: GCC `--coverage` with `gcovr` (CI) .
- Continuous integration: GitHub Actions workflow `.github/workflows/ci.yml` builds, runs all tests and fails the build if business-logic line coverage drops below 70%.

## 6. Traceability: case-study rules to tests
Link these rule numbers to the functional requirement IDs of your SRS in the traceability matrix.

| Rule | Description | Main tests |
|---|---|---|
| R1 | Log in with username and password; each role does only its own actions | `AuthTest.Login*`, `Permissions.*`, `SystemTest.EachRoleSeesOnlyItsOwnMenu`, `SystemTest.LoginWrongPasswordThenCorrect` |
| R2 | Passwords stored as salted hashes | `PasswordHasher.*`, `AuthTest.PlainTextPasswordIsNeverStored`, `AuthTest.SamePasswordGetsDifferentHashes...` |
| R3 | Administrator manages users, departments, courses, lecturers, enrolment | `CourseServiceTest.*`, `AuthTest.OnlyAdministratorsCreateAccounts`, `SystemTest.AdministratorCan*` |
| R4 | Lecturer creates assignments with deadline and maximum mark | `AssignmentServiceTest.*` |
| R5 | Lecturer grants individual extensions | `AssignmentServiceTest.Extension*`, `SubmissionTest.Extension*`, `AcceptanceTest.Lecturer_*` |
| R6 | Student submits and may resubmit before the deadline | `SubmissionTest.Resubmission*`, `SystemTest.SubmitAssignment*` |
| R7 | Late work is accepted and flagged, never rejected | `SubmissionTest.LateWorkIsAccepted...`, `AcceptanceTest.StudentRep_*` |
| R8 | Late penalty: 10% per day for up to 3 days, then zero | `TenPercentPolicy.*`, `SubmissionTest.LatePenaltyBoundaryValues`, `AcceptanceTest.Hod_TenPercentPerDay...` |
| R9 | Lecturer marks and writes feedback | `SubmissionTest.MarkRangeBoundaries`, `SubmissionTest.WrittenFeedbackIsRequired`, `SubmissionTest.OnlyTheLecturerOfTheCourseMayMark` |
| R10 | HoD reviews, moderates and releases marks of own department only | `ModerationTest.*`, `AcceptanceTest.Hod_SeesEveryMark...`, `SystemTest.FullModerationFlow...`, `SystemTest.HodOfAnotherDepartmentIsRefused` |
| R11 | Students see marks only after the HoD has released them | `SubmissionTest.StudentsSeeMarksOnlyAfterRelease`, `AcceptanceTest.Principal_*`, `RepositoryTest.SubmissionQueries...` |
| R12 | Submission life cycle (submitted, late, marked, moderated, released) | `SubmissionLifecycle.*` |
| R13 | Every mark change is audited (who, when, old, new, why) | `SubmissionTest.EveryMarkChangeIsAudited...`, `ModerationTest.AdjustMark...`, `AcceptanceTest.School_*`, `RepositoryTest.AuditLogIsAppendOnly`, `SubmissionTest.MarkAndAuditEntryAreAtomic` |
| R14 | Persistent storage in SQLite through a data-access layer, parameterised queries | `RepositoryTest.*`, `DatabaseTest.*`, `AuthTest.SqlInjectionInLoginFails`, `RepositoryTest.MaliciousText...` |
| R15 | Invalid input or database errors do not crash the program | `InputValidator.*`, `SystemTest.*InvalidInput*`, `SystemTest.*DatabaseFailure*`, `SystemTest.ClosedInput...` |
| R16 | Performance with 800 students: login within 2 s | `Performance.*` |
| R17 | Works offline | Manual test MT-02 (the program contains no network code) |

## 7. Manual tests (not automated)
| ID | Test | Expected result | Why not automated |
|---|---|---|---|
| MT-01 | Start `./build/lms` in a real terminal; type a password at the prompt | Characters are not shown | `StdConsole` talks to a real terminal; tests use `ScriptedConsole` |
| MT-02 | Disconnect the computer from the network, then run the full workflow | Everything works | Absence of network use is a property of the whole program |
| MT-03 | Run the program, create the first administrator, quit, run again, log in | Account persists between runs | Covers the real `lms.db` file path |
| MT-04 | Press Ctrl+D at a prompt | Program says "Goodbye" and exits cleanly | Real end-of-input on a terminal |

## 8. Defect handling
Every failure is written to `DEFECT_LOG.md` with severity, cause, fix and retest result. Severity: Critical (data loss, security breach, crash), High (a Must requirement fails), Medium (wrong behaviour in an edge case), Low (poor message or cosmetic). A fix is accepted only when a regression test that failed before the fix passes after it, and the whole suite is rerun.

## 9. Risks to testing
- Deadline logic is time-dependent: mitigated with `FixedClock` and second-level boundary tests.
- Production password hashing is slow: mitigated with a reduced cost in tests and one timed test at production cost.
- Tests share no state: each test builds its own in-memory database.
