# Coverage report

## Method
Line coverage measured with GCC `--coverage` and merged across all test executables (a line counts as covered if any test runs it).
In CI the same figure is produced by `gcovr` (see `.github/workflows/ci.yml`), which also fails the build if the business-logic layer drops below 70%.
`main.cpp` (the composition root that only creates objects) is excluded.
Figures below were measured on the development machine; replace them with the numbers from your own CI run before submission.

## Result by layer
| Layer | Lines covered | Lines | Coverage |
|---|---|---|---|
| Domain | 249 | 252 | 98.8% |
| Service | 317 | 321 | 98.8% |
| **Business logic (domain + service)** | **566** | **573** | **98.8%** (target: at least 70%) |
| Data access | 305 | 307 | 99.3% |
| Presentation (menus, validator, console) | 312 | 342 | 91.2% |
| **All code except main.cpp** | **1183** | **1222** | **96.8%** |

## Result by file
| File | Lines covered | Lines | Coverage |
|---|---|---|---|
| `data/Database.cpp` | 59 | 61 | 96.7% |
| `data/Database.h` | 1 | 1 | 100.0% |
| `data/ITransactionManager.h` | 7 | 7 | 100.0% |
| `data/Repositories.h` | 5 | 5 | 100.0% |
| `data/SqliteRepositories.cpp` | 228 | 228 | 100.0% |
| `data/SqliteRepositories.h` | 5 | 5 | 100.0% |
| `domain/Entities.h` | 45 | 45 | 100.0% |
| `domain/IClock.cpp` | 6 | 6 | 100.0% |
| `domain/IClock.h` | 5 | 5 | 100.0% |
| `domain/LatePenaltyPolicy.cpp` | 8 | 8 | 100.0% |
| `domain/LatePenaltyPolicy.h` | 5 | 5 | 100.0% |
| `domain/Submission.cpp` | 42 | 42 | 100.0% |
| `domain/Submission.h` | 10 | 10 | 100.0% |
| `domain/Types.cpp` | 71 | 73 | 97.3% |
| `domain/User.cpp` | 8 | 8 | 100.0% |
| `domain/User.h` | 32 | 32 | 100.0% |
| `domain/UserFactory.cpp` | 17 | 18 | 94.4% |
| `service/AssignmentService.cpp` | 46 | 47 | 97.9% |
| `service/AssignmentService.h` | 2 | 2 | 100.0% |
| `service/AuditService.cpp` | 9 | 9 | 100.0% |
| `service/AuditService.h` | 1 | 1 | 100.0% |
| `service/AuthService.cpp` | 53 | 53 | 100.0% |
| `service/AuthService.h` | 1 | 1 | 100.0% |
| `service/Authorization.h` | 4 | 4 | 100.0% |
| `service/CourseService.cpp` | 47 | 48 | 97.9% |
| `service/CourseService.h` | 2 | 2 | 100.0% |
| `service/Deadlines.h` | 5 | 5 | 100.0% |
| `service/ModerationService.cpp` | 53 | 53 | 100.0% |
| `service/ModerationService.h` | 2 | 2 | 100.0% |
| `service/PasswordHasher.cpp` | 28 | 30 | 93.3% |
| `service/SubmissionService.cpp` | 61 | 61 | 100.0% |
| `service/SubmissionService.h` | 3 | 3 | 100.0% |
| `ui/Console.cpp` | 0 | 25 | 0.0% |
| `ui/Console.h` | 1 | 1 | 100.0% |
| `ui/InputValidator.cpp` | 34 | 34 | 100.0% |
| `ui/Menu.cpp` | 260 | 265 | 98.1% |
| `ui/Menu.h` | 17 | 17 | 100.0% |

## Lines that are not covered, and why
| Where | What | Reason |
|---|---|---|
| `domain/Types.cpp` (2 lines) | `return "UNKNOWN"` after an exhaustive `switch` | Unreachable unless a new enum value is added without updating the function; kept as a compiler-warning guard |
| `domain/UserFactory.cpp` (1 line) | `throw ... "Unknown role."` after the `switch` | Same: unreachable with the current four roles |
| `service/CourseService.cpp` (1 line) | `return {}` after the `switch` in `coursesFor` | Same |
| `service/PasswordHasher.cpp` (2 lines) | Failure path when libsodium runs out of memory; `return false` when the hash call fails during verify | Cannot be triggered without exhausting memory |
| `service/AssignmentService.cpp` (1 line) | Closing brace attributed to an unreachable path by the compiler | Not executable code |
| `data/Database.cpp` (2 lines) | Move constructor of `Statement`; failure to bind text | Move is optimised away by the compiler; bind failure needs a corrupted database handle |
| `ui/Console.cpp` (25 lines, 0%) | Real terminal input and password echo control | Tests replace the console with `ScriptedConsole`; covered by manual tests MT-01 and MT-04 in the test plan |
| `ui/Menu.cpp` (5 lines) | The catch-all branches for an unexpected non-`LmsException` error (menu loop and log-in loop), and the re-throw when input closes during first-run set-up | The catch-all needs a bug to throw such an exception; the first-run re-throw is exercised only with a real terminal (manual test MT-04) |
