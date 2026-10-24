// Basic value types shared by every layer: enumerations and date/time helpers.
#pragma once
#include <cstdint>
#include <string>

namespace lms {

// Seconds since 1970-01-01 00:00:00 of the *school's local wall-clock time*
// (naive, no time zone or daylight-saving handling). See README, "Time handling".
using DateTime = std::int64_t;

constexpr DateTime kSecondsPerDay = 86400;

enum class Role { Student, Lecturer, HeadOfDepartment, Administrator };

enum class Permission {
    SubmitWork,
    ViewReleasedMarks,
    CreateAssignment,
    GrantExtension,
    MarkSubmission,
    ModerateMarks,
    ReleaseMarks,
    ManageUsers,
    ManageCourses
};

enum class SubmissionStatus { Submitted, Late, Marked, Moderated, Released };

std::string toString(Role role);
Role roleFromString(const std::string& text);
std::string toString(SubmissionStatus status);
SubmissionStatus statusFromString(const std::string& text);

// "YYYY-MM-DD HH:MM:SS"
std::string formatDateTime(DateTime t);
// Accepts "YYYY-MM-DD HH:MM" or "YYYY-MM-DD HH:MM:SS"; throws ValidationException.
DateTime parseDateTime(const std::string& text);

}  // namespace lms
