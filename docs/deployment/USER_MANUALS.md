# User Manuals — Moderated LMS v1.0

Four roles use the system: Student, Lecturer, Head of Department (HoD) and Administrator. Every user logs in with a username and password and can only perform the actions allowed by their role.

The menu numbers below follow the system's console menus. If your build shows slightly different numbering, the option names are the same.

## 1. Student manual

### Logging in
1. Run lms (or ask the technician to start it).
2. Enter your username and password.
3. The Student menu appears.

### Viewing courses and assignments
- Choose the option to view my courses to see the courses you are enrolled on.
- Choose view my assignments to see each assignment's title, deadline and maximum mark.

### Submitting an assignment
1. Choose submit assignment.
2. Select the assignment.
3. Type or paste your work / confirm the submission.
4. The submission is recorded as Submitted (before the deadline) or Late (after the deadline). Late work is never rejected.

### Resubmitting
- You may resubmit before the deadline. The earlier submission is replaced.
- After the deadline you cannot resubmit unless the lecturer has granted you an extension.

### Viewing marks and feedback
- Marks are not visible until the Head of Department has moderated and released them.
- Choose view released marks to see your mark and written feedback once released.
- If your mark has not been released, the assignment shows as "not yet released".

## 2. Lecturer manual

### Logging in
Log in with your lecturer account to open the Lecturer menu.

### Creating an assignment
1. Choose create assignment.
2. Select the course.
3. Enter the title, deadline (YYYY-MM-DD HH:MM), and the maximum mark.
4. The assignment is now visible to enrolled students.

### Granting an extension
1. Choose grant extension.
2. Select the assignment and the student.
3. Enter the new deadline.
4. The student's effective deadline changes; the assignment's original deadline is unchanged.

Note: In v1.0 extensions apply to individual students only. Class-wide extensions are introduced in v1.1 (CR1).

### Marking submissions
1. Choose mark submissions.
2. Select the course and assignment.
3. Choose a submission and enter the mark and written feedback.
4. A late penalty is applied automatically: 10% of the maximum mark per day late, up to 3 days, then zero.
5. If you change an existing mark you must give a reason; the change is recorded in the audit log.

### Viewing marks
- Your marks are visible to you, but students do not see them until the HoD releases them.

## 3. Head of Department (HoD) manual

### Logging in
Log in with your HoD account to open the HoD menu.

### Reviewing marks
1. Choose review marks.
2. Select one of the courses in your department.
3. View every submission's mark, late penalty and feedback. You can only see courses in your own department.

### Moderating a mark
1. Choose moderate mark.
2. Select the submission.
3. Enter the adjusted mark and a reason.
4. The change is recorded in the audit log (who, when, old value, new value, reason).

### Releasing marks
1. Choose release marks.
2. Select the course and confirm.
3. Released marks and feedback become visible to students. Students never see a mark before it is released.

## 4. Administrator manual

### Logging in
The first run of the system asks you to create the administrator account. Log in with it to open the Administrator menu.

### Managing users
- Create user: choose the role (student, lecturer, HoD, administrator), enter username and password.
- Disable / enable user: prevents or restores login.
- List users: shows all accounts.

### Managing departments and courses
- Create department: enter the department name.
- Create course: choose the department, enter the course code and title.
- Assign lecturer to course: select a lecturer and a course.
- Enrol student: select a student and a course.

### Audit log
- Choose view audit log to see every recorded mark change: who made it, when, the old and new values, and the reason. Audit records cannot be edited or deleted.

### Backups
Backups are handled by the technician (see INSTALLATION_GUIDE.md section 7). Ask the technician to restore a backup if data is lost.
