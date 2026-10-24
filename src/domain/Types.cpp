#include "Types.h"

#include <cstdio>

#include "Exceptions.h"

namespace lms {
namespace {

// Howard Hinnant's civil-calendar algorithms: portable, no <ctime> time-zone surprises.
std::int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

void civilFromDays(std::int64_t z, int& y, unsigned& m, unsigned& d) {
    z += 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp + (mp < 10 ? 3 : -9);
    y += m <= 2;
}

}  // namespace

std::string toString(Role role) {
    switch (role) {
        case Role::Student: return "STUDENT";
        case Role::Lecturer: return "LECTURER";
        case Role::HeadOfDepartment: return "HEAD_OF_DEPARTMENT";
        case Role::Administrator: return "ADMINISTRATOR";
    }
    return "UNKNOWN";
}

Role roleFromString(const std::string& text) {
    if (text == "STUDENT") return Role::Student;
    if (text == "LECTURER") return Role::Lecturer;
    if (text == "HEAD_OF_DEPARTMENT") return Role::HeadOfDepartment;
    if (text == "ADMINISTRATOR") return Role::Administrator;
    throw ValidationException("Unknown role: " + text);
}

std::string toString(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::Submitted: return "SUBMITTED";
        case SubmissionStatus::Late: return "LATE";
        case SubmissionStatus::Marked: return "MARKED";
        case SubmissionStatus::Moderated: return "MODERATED";
        case SubmissionStatus::Released: return "RELEASED";
    }
    return "UNKNOWN";
}

SubmissionStatus statusFromString(const std::string& text) {
    if (text == "SUBMITTED") return SubmissionStatus::Submitted;
    if (text == "LATE") return SubmissionStatus::Late;
    if (text == "MARKED") return SubmissionStatus::Marked;
    if (text == "MODERATED") return SubmissionStatus::Moderated;
    if (text == "RELEASED") return SubmissionStatus::Released;
    throw ValidationException("Unknown submission status: " + text);
}

std::string formatDateTime(DateTime t) {
    std::int64_t days = t / kSecondsPerDay;
    std::int64_t rem = t % kSecondsPerDay;
    if (rem < 0) { rem += kSecondsPerDay; --days; }
    int y; unsigned m, d;
    civilFromDays(days, y, m, d);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02u-%02u %02d:%02d:%02d", y, m, d,
                  static_cast<int>(rem / 3600), static_cast<int>(rem % 3600 / 60), static_cast<int>(rem % 60));
    return buf;
}

DateTime parseDateTime(const std::string& text) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    int consumed = 0;
    int n = std::sscanf(text.c_str(), "%d-%d-%d %d:%d:%d%n", &y, &mo, &d, &h, &mi, &s, &consumed);
    if (n != 6) {
        consumed = 0;
        n = std::sscanf(text.c_str(), "%d-%d-%d %d:%d%n", &y, &mo, &d, &h, &mi, &consumed);
        s = 0;
        if (n != 5) throw ValidationException("Date/time must look like 2026-11-30 23:59");
    }
    if (static_cast<std::size_t>(consumed) != text.size())
        throw ValidationException("Date/time must look like 2026-11-30 23:59");
    const DateTime t = daysFromCivil(y, static_cast<unsigned>(mo), static_cast<unsigned>(d)) * kSecondsPerDay +
                       h * 3600 + mi * 60 + s;
    // Round-trip check rejects impossible dates such as 2026-02-31 or 25:00.
    std::string canonical = formatDateTime(t);
    char expect[32];
    std::snprintf(expect, sizeof expect, "%04d-%02d-%02d %02d:%02d:%02d", y, mo, d, h, mi, s);
    if (canonical != expect) throw ValidationException("That date or time does not exist.");
    return t;
}

}  // namespace lms
