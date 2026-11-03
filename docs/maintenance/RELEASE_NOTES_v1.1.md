# Release Notes — Moderated LMS v1.1

**Release date:** 7th November 2026  
**Tag:** v1.1  
**Platform:** Ubuntu Linux 22.04 or later; single school computer, fully offline.

## Summary
Version 1.1 implements Change Request 1 (CR1). This is a **perfective maintenance** release that enhances the extension system by allowing lecturers to apply a deadline extension to an entire class at once, and improves student visibility by displaying their active extensions on their assignment list.

## Changes in v1.1
### New Features
- **Class-wide extensions:** Lecturers now have a new menu option ("Grant an extension to the whole class"). This applies a single new deadline and reason to every student enrolled in the selected course.
- **Extensions visible to students:** When a student views their assignment list, any active extension (individual or class-wide) is now displayed, showing the new deadline and the reason given by the lecturer.

### Technical Improvements
- Added `findStudentIdsByCourse` to the `ICourseRepository` interface to support fetching enrolled students for a course.
- Added `grantClassExtension` and `extensionFor` to `AssignmentService`.
- Class-wide extensions are executed inside a database transaction to guarantee atomicity (either all students get the extension, or none do).
- Injected `ITransactionManager` into `AssignmentService` to allow multi-row database operations.

### Regression Testing
- The full automated test suite has grown from 164 to **170 tests**.
- All 164 original tests passed unchanged, verifying that individual extensions, late penalties, submission life cycles, moderation, and auditing continue to work exactly as they did in v1.0.
- 6 new tests were added to cover class-wide extensions, role permissions, validation rules (e.g., rejecting deadlines that are not later than the original), and the student visibility feature.

### Known Limitations
- Class-wide extensions apply to all currently enrolled students. If a student is enrolled *after* the class extension is granted, they will not automatically receive the extension and will need an individual extension.
- Console interface only; some Windows consoles echo passwords while typing.
