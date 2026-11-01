# Test results: release 1.0

## 1. How these results were produced
- Build: GCC 13.3 on Ubuntu 24.04, `-std=c++17 -Wall -Wextra -Wpedantic -Wshadow -Wconversion`, no warnings in the source or the tests.
- Runner: the tests were run with a minimal GoogleTest-compatible runner because the build machine had no GoogleTest installed. The test code itself uses only standard GoogleTest macros (`TEST`, `TEST_F`, `EXPECT_*`, `ASSERT_*`), so it builds unchanged against the real library.
- **Before submitting, run the suite yourself with real GoogleTest** (`ctest`, see section 5) and the CI workflow, then copy your own totals and the CI run link into section 6. The raw output of the run below is in `test_run_output_sandbox.txt`.

## 2. Summary
| Measure | Result |
|---|---|
| Automated tests | **164** |
| Passed | **164** |
| Failed | 0 |
| Line coverage, business-logic layer (`src/domain` + `src/service`) | **98.8%** (566 of 573 lines); target 70% |
| Line coverage, all code except `main.cpp` | 96.8% (1183 of 1222 lines) |
| Defects found and fixed | 3 (see DEFECT_LOG.md) |

## 3. Results by level
| Level | Tests | Suites |
|---|---|---|
| Unit (domain, helpers) | 41 | DateTime (6), Enums (2), Clock (2), TenPercentPolicy (7), NoPenaltyPolicy (1), Deadlines (2), Entities (1), SubmissionLifecycle (7), Permissions (1), Users (1), UserFactory (2), PasswordHasher (6), InputValidator (3) |
| Integration (repositories, services with real SQLite) | 94 | RepositoryTest (21), DatabaseTest (4), AuthTest (15), AuthSetup (1), CourseServiceTest (6), AssignmentServiceTest (11), SubmissionTest (27), ModerationTest (9) |
| System (use-case flows through the menus) | 21 | SystemFirstRun (1), SystemTest (20) |
| Acceptance (stakeholder statements and NFRs) | 8 | AcceptanceTest (6), Performance (2) |
| **Total** | **164** | |

Every test passed on the final run.

## 4. Non-functional measurements (production password hashing, 800 users)
| Requirement | Limit | Measured | Test |
|---|---|---|---|
| Log in with 801 users stored | under 2 s | 0.05 s | `Performance.LoginWithin2SecondsWith800UsersStoredUsingProductionHashing` |
| Review 800 marks awaiting moderation | under 1 s | 0.001 s | `Performance.ReviewingAndReleasing800SubmissionsIsFast` |
| Release 800 approved marks | under 2 s | 0.007 s | same |

These numbers come from one development machine. Record the numbers from the school computer (or the slowest computer you can find) in your report, because the IT officer's computers are old.

## 5. How to reproduce
```bash
sudo apt install build-essential cmake pkg-config libsqlite3-dev libsodium-dev gcovr
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DLMS_BUILD_TESTS=ON -DLMS_COVERAGE=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
gcovr -r . --filter 'src/domain/' --filter 'src/service/' --txt --print-summary
```

## 6. Results of my own run (GitHub Actions)

| Item | Value |
|---|---|
| Date and machine | 2026-11-01, GitHub Actions (Ubuntu); local build on HP ProBook 450 G7 |
| `ctest` result (passed / total) | 164 / 164 |
| Business-logic coverage reported by gcovr | PUT-THE-PERCENTAGE-FROM-THE-CI-LOG-HERE |
| Link to the successful CI run on GitHub | https://github.com/pokathasaku02-create/Software-Engineering/actions/runs/41310941981/job/124860977396 |

## 7. Do the tests actually catch bugs? (mutation check)
A test suite that never fails proves little, so I deliberately broke the code in ten ways and ran the suite each time.

| # | Deliberate bug | Tests that failed |
|---|---|---|
| 1 | Penalty rate 10% changed to 20% | 9 |
| 2 | Grace period 3 days changed to 4 days | 4 |
| 3 | Late check `now > deadline` changed to `now >= deadline` | 3 |
| 4 | Enrolment check removed from submit | 2 |
| 5 | Students can see `MARKED` marks, not just `RELEASED` | 4 |
| 6 | Head of Department department check removed | 2 |
| 7 | Password check replaced by plain-text comparison | 18 |
| 8 | Re-introduce DEF-002 (Late cannot be resubmitted) | 10 |
| 9 | `submittedAt <= deadline` changed to `<` in `daysLate` | 0, equivalent mutant: the result is identical because 0 seconds late rounds to 0 days either way |
| 10 | "Reason required" check removed when re-marking | 0, equivalent in behaviour: `AuditService` independently refuses an empty reason, so the same error still occurs (defence in depth) |

(A further experiment that damaged the SQL schema made every test fail and is not counted.) Seven of the seven non-equivalent mutants were caught.

## 8. Manual test results

| ID | Test | Date | Result |
|---|---|---|---|
| MT-01 | Password is hidden when typed | 2026-11-01 | Pass |
| MT-02 | Works with the network disconnected | 2026-11-01 | Pass |
| MT-03 | Data persists between runs | 2026-11-01 | Pass |
| MT-04 | Ctrl+D ends the program cleanly | 2026-11-01 | Pass |
