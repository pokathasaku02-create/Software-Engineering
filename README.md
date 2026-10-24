# Moderated LMS (CS4433 final project)

A console Learning Management System for a secondary school: students submit work, lecturers mark it,
Heads of Department moderate and release marks, and every mark change is audited.
C++17, SQLite for storage, libsodium (Argon2id) for password hashing. Works fully offline.

## 1. Build guide

### Requirements
- A C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.16+
- SQLite 3 development files and libsodium development files

### Ubuntu / Debian
```bash
sudo apt install build-essential cmake pkg-config libsqlite3-dev libsodium-dev
git clone <your-repository-url> lms && cd lms
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/lms            # uses ./lms.db, created on first run
./build/lms /path/to/school.db
```

### Windows (vcpkg)
```
vcpkg install sqlite3:x64-windows libsodium:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```
(On Windows the password prompt echoes what is typed; on Linux/macOS it does not.)

### First run
If the database has no users, the program asks you to create the administrator account.
There is **no default password**. Log in as that administrator and create departments, courses,
lecturers, heads of department and students from the Administrator menu.

## 2. How the code follows the design

| Layer (architecture figure) | Folder | Main classes |
|---|---|---|
| Presentation | `src/ui` | `Menu`, `StudentMenu`, `LecturerMenu`, `HoDMenu`, `AdminMenu`, `LoginMenu`, `InputValidator`, `Console` |
| Service | `src/service` | `AuthService`, `PasswordHasher`, `CourseService`, `AssignmentService`, `SubmissionService`, `ModerationService`, `AuditService` |
| Domain | `src/domain` | `User` (abstract), `Student`, `Lecturer`, `HeadOfDepartment`, `Administrator`, `UserFactory`, `Submission`, `Entities.h`, `LatePenaltyPolicy` |
| Data access | `src/data` | `Repositories.h` (interfaces), `SqliteRepositories.*`, `Database`, `Schema.h` |
| Composition root | `src/main.cpp` | builds the object graph; the only file that names every concrete class |

Dependencies point downward only. Services use repository **interfaces**, never SQLite.

### Design patterns
- **Strategy**: `LatePenaltyPolicy` with `TenPercentPerDayPolicy` (10% of the maximum mark per day, up to 3 days, zero after) and `NoPenaltyPolicy`.
- **Repository**: five `I...Repository` interfaces, each with a `Sqlite...Repository`.
- **Factory**: `UserFactory::create` is the only code that constructs `Student`, `Lecturer`, `HeadOfDepartment` or `Administrator`.

### Business rules (where they live)
| Rule | Implemented in |
|---|---|
| Late work is accepted and flagged LATE, never rejected | `SubmissionService::submit` |
| Resubmission only before the effective deadline; refused when late | `SubmissionService::submit`, `Submission::resubmit` |
| Extension changes the student's effective deadline | `Deadlines.h`, `AssignmentService::grantExtension` |
| Penalty: 10% of maxMark per day late, up to 3 days, then zero | `TenPercentPerDayPolicy`, applied in `SubmissionService::markSubmission` |
| Students see marks only when RELEASED | `Submission` state machine, `SubmissionService::releasedMarksFor` |
| HoD moderates only courses of their own department | `ModerationService` |
| Every mark change is audited (who, when, old, new, reason) in the same transaction | `AuditService`, `ScopedTransaction`; the table has triggers that forbid UPDATE and DELETE |
| Role-based access | `User::canPerform` (polymorphic) checked in every service method |

### Refinements made while implementing (the diagrams were updated to match)
- Services receive the acting `User&` (not a bare id) so they can check role and department themselves.
- `markSubmission` takes a `reason`, required when an existing mark is changed.
- Extra repository operations the menus needed: `findAll`, `update`, `findDepartment(s)` on `ICourseRepository`.
- `IClock` (injectable time) and `ITransactionManager` (implemented by `Database`) are infrastructure interfaces.
- `Submission::adjustFinalMark` supports the Head of Department's moderation change.
- Extensions are **not** shown on the student's assignment list in v1.0 (that is change request CR1).

## 3. Security
- **Passwords**: only a salted Argon2id hash is stored (`libsodium`, `crypto_pwhash`, 16-byte random salt per user, hex encoded). Comparison is constant-time (`sodium_memcmp`).
  Argon2id is the Password Hashing Competition winner and the algorithm OWASP recommends first; libsodium is a widely audited library, so no cryptography is written by hand.
  The default cost (2 passes, 64 MB) suits a school PC; lower `crypto_pwhash_MEMLIMIT_*` in `PasswordHasher.cpp` for very old machines.
- **SQL injection**: every query is a prepared statement with bound parameters.
- **Login**: one generic error message for unknown user, wrong password and disabled account; hashing work is also done for unknown usernames to reduce timing differences.
- **Input**: all menu input passes through `InputValidator` (length, type, range, control-character stripping) and services validate again.
- **Errors**: any exception is caught in the menu loop, so bad input or a database error never crashes the program.

## 4. Time handling
Dates are entered and shown as `YYYY-MM-DD HH:MM` in the school computer's local time. Internally they are
stored as text `YYYY-MM-DD HH:MM:SS` with no time zone or daylight-saving conversion (one school, one clock).
"Days late" are whole days rounded up: 1 second late counts as 1 day late.

## 5. Known limitations
- No account lock-out after repeated failed logins, and no password-change screen (both are sensible future changes).
- Console only; the Windows console echoes passwords.
- The marks are whole numbers.
