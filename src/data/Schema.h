// SQL schema (third normal form). See docs/data_dictionary for column descriptions.
#pragma once

namespace lms {

constexpr const char* kSchemaSql = R"SQL(
CREATE TABLE IF NOT EXISTS department (
    department_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name          TEXT NOT NULL UNIQUE CHECK (length(name) > 0)
);

CREATE TABLE IF NOT EXISTS users (
    user_id       INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE CHECK (length(username) > 0),
    password_hash TEXT NOT NULL,
    salt          TEXT NOT NULL,
    full_name     TEXT NOT NULL,
    role          TEXT NOT NULL CHECK (role IN ('STUDENT','LECTURER','HEAD_OF_DEPARTMENT','ADMINISTRATOR')),
    ref_number    TEXT NOT NULL DEFAULT '',
    department_id INTEGER REFERENCES department(department_id),
    is_active     INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0,1))
);
-- At most one Head of Department per department.
CREATE UNIQUE INDEX IF NOT EXISTS ux_one_hod_per_department
    ON users(department_id) WHERE role = 'HEAD_OF_DEPARTMENT';

CREATE TABLE IF NOT EXISTS course (
    course_id     INTEGER PRIMARY KEY AUTOINCREMENT,
    code          TEXT NOT NULL UNIQUE CHECK (length(code) > 0),
    title         TEXT NOT NULL,
    department_id INTEGER NOT NULL REFERENCES department(department_id),
    lecturer_id   INTEGER REFERENCES users(user_id)
);

CREATE TABLE IF NOT EXISTS enrolment (
    student_id  INTEGER NOT NULL REFERENCES users(user_id),
    course_id   INTEGER NOT NULL REFERENCES course(course_id),
    enrolled_on TEXT NOT NULL,
    PRIMARY KEY (student_id, course_id)
);

CREATE TABLE IF NOT EXISTS assignment (
    assignment_id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_id     INTEGER NOT NULL REFERENCES course(course_id),
    title         TEXT NOT NULL,
    description   TEXT NOT NULL DEFAULT '',
    deadline      TEXT NOT NULL,
    max_mark      INTEGER NOT NULL CHECK (max_mark > 0)
);

CREATE TABLE IF NOT EXISTS extension (
    extension_id  INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id INTEGER NOT NULL REFERENCES assignment(assignment_id),
    student_id    INTEGER NOT NULL REFERENCES users(user_id),
    new_deadline  TEXT NOT NULL,
    reason        TEXT NOT NULL,
    granted_by    INTEGER NOT NULL REFERENCES users(user_id),
    granted_at    TEXT NOT NULL,
    UNIQUE (assignment_id, student_id)
);

CREATE TABLE IF NOT EXISTS submission (
    submission_id  INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id  INTEGER NOT NULL REFERENCES assignment(assignment_id),
    student_id     INTEGER NOT NULL REFERENCES users(user_id),
    content        TEXT NOT NULL,
    submitted_at   TEXT NOT NULL,
    resubmit_count INTEGER NOT NULL DEFAULT 0 CHECK (resubmit_count >= 0),
    status         TEXT NOT NULL CHECK (status IN ('SUBMITTED','LATE','MARKED','MODERATED','RELEASED')),
    raw_mark       INTEGER CHECK (raw_mark >= 0),
    final_mark     INTEGER CHECK (final_mark >= 0),
    feedback       TEXT NOT NULL DEFAULT '',
    UNIQUE (assignment_id, student_id)
);
CREATE INDEX IF NOT EXISTS ix_submission_assignment ON submission(assignment_id);
CREATE INDEX IF NOT EXISTS ix_submission_student ON submission(student_id);

CREATE TABLE IF NOT EXISTS audit_entry (
    audit_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    submission_id INTEGER NOT NULL REFERENCES submission(submission_id),
    changed_by    INTEGER NOT NULL REFERENCES users(user_id),
    changed_at    TEXT NOT NULL,
    old_value     INTEGER,
    new_value     INTEGER NOT NULL,
    reason        TEXT NOT NULL CHECK (length(reason) > 0)
);
CREATE INDEX IF NOT EXISTS ix_audit_submission ON audit_entry(submission_id);

-- The audit log is append-only: rows can never be changed or removed.
CREATE TRIGGER IF NOT EXISTS audit_no_update BEFORE UPDATE ON audit_entry
BEGIN SELECT RAISE(ABORT, 'audit_entry is append-only'); END;
CREATE TRIGGER IF NOT EXISTS audit_no_delete BEFORE DELETE ON audit_entry
BEGIN SELECT RAISE(ABORT, 'audit_entry is append-only'); END;
)SQL";

}  // namespace lms
