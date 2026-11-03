# Change Request 1 (CR1) Analysis

## 1. Change Request Description
**Original Request:** "A lecturer should be able to extend the deadline for a whole class, not just an individual student, and extensions should be shown on the student's assignment list."

## 2. Classification
**Maintenance Type:** Perfective Maintenance.

**Justification:** The system is not being corrected for a defect (corrective), nor is it being adapted to a new environment (adaptive). Instead, the change request asks for an enhancement to existing functionality. The system already supports individual extensions; CR1 refines this feature to support bulk operations (class-wide) and improves the user interface by making extensions visible to students on their assignment list.

## 3. Impact Analysis

### 3.1 Affected Requirements
- **FR-EXT-01 (Existing):** Lecturer grants an extension to an individual student.
  - *Impact:* Remains valid. No breaking changes to this requirement.
- **FR-EXT-02 (New):** Lecturer grants a deadline extension to an entire class (all students enrolled in a course).
- **FR-STU-01 (Modified):** Student views assignment list.
  - *Impact:* The list must now also display active extensions (the new deadline and the reason) for that student.

### 3.2 Affected Design Documents
- **Class Diagram (Figure 3a/3b):**
  - `AssignmentService` gains two methods: `grantClassExtension()` and `extensionFor()`.
  - `AssignmentService` gains a dependency on `ITransactionManager`.
  - `ICourseRepository` (interface) gains `findStudentIdsByCourse()`.
- **Database Design:**
  - No schema changes required. The `extension` table already supports multiple rows per assignment (one per student). Class-wide extensions are handled as a batch transaction of individual extension records.
- **User Manuals:**
  - Lecturer manual updated to include option 7 ("Grant an extension to the whole class").
  - Student manual updated to mention that extensions are shown on the assignment list.
- **Installation Guide / Deployment Diagram:**
  - No impact.

### 3.3 Affected Code (Classes & Files)
- `src/data/Repositories.h`: Added `findStudentIdsByCourse(int)` to `ICourseRepository`.
- `src/data/SqliteRepositories.h` / `.cpp`: Implemented `findStudentIdsByCourse(int)` in `SqliteCourseRepository`.
- `src/service/AssignmentService.h` / `.cpp`: 
  - Added `ClassExtensionData` struct.
  - Added `ITransactionManager& tx_` dependency.
  - Added `grantClassExtension()` and `extensionFor()` methods.
- `src/ui/Menu.h` / `.cpp`: 
  - Added option 7 to `LecturerMenu`.
  - Updated `StudentMenu` assignment list to display extensions.
- `src/main.cpp`: Updated `AssignmentService` instantiation to pass `db` (which implements `ITransactionManager`).

### 3.4 Affected Database Tables
- **extension:** Used for bulk inserts during class-wide extensions.
- **enrolment:** Read-only access via the new `findStudentIdsByCourse` query to identify which students receive the class-wide extension.

### 3.5 Affected Tests
- **Existing Tests:** All 164 existing tests passed without modification, proving that the existing individual extension feature and all other business rules remain intact (Regression Testing).
- **New Tests Added:** 6 new unit/integration tests were added to `tests/test_submission_moderation.cpp`:
  1. `ClassExtensionCoversEveryEnrolledStudent`
  2. `ClassExtensionRejectsDeadlineNotLaterThanAssignment`
  3. `ClassExtensionRequiresReason`
  4. `OnlyTheLecturerOfTheCourseMayGrantAClassExtension`
  5. `ClassExtensionIsReplacedByASubsequentClassExtension`
  6. `ExtensionForIsOnlyVisibleToTheStudentItBelongsTo`

## 4. Implementation Summary
The class-wide extension is implemented atomically using a `ScopedTransaction`. When a lecturer requests a class extension, the service fetches all student IDs enrolled in the course, iterates through them, and calls the existing `saveExtension()` repository method (which performs an upsert). If any single insert fails, the whole transaction rolls back, preventing partial class extensions. The student assignment list was updated to query the student's active extension for each assignment and display it.
