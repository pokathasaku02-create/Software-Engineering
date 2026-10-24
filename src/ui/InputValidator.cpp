#include "InputValidator.h"

#include <cctype>
#include <climits>
#include <cstdlib>

#include "../domain/Exceptions.h"
#include "../service/AuthService.h"

namespace lms {
namespace {

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

}  // namespace

std::string InputValidator::text(const std::string& raw, const std::string& field, std::size_t minLen, std::size_t maxLen) {
    std::string clean;
    for (unsigned char c : trim(raw))
        if (!std::iscntrl(c)) clean.push_back(static_cast<char>(c));
    if (clean.size() < minLen || clean.size() > maxLen)
        throw ValidationException(field + " must have " + std::to_string(minLen) + " to " + std::to_string(maxLen) + " characters.");
    return clean;
}

int InputValidator::integer(const std::string& raw, const std::string& field, int min, int max) {
    const std::string s = trim(raw);
    if (s.empty() || s.size() > 10) throw ValidationException(field + " must be a whole number.");
    std::size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i == s.size()) throw ValidationException(field + " must be a whole number.");
    for (std::size_t k = i; k < s.size(); ++k)
        if (!std::isdigit(static_cast<unsigned char>(s[k]))) throw ValidationException(field + " must be a whole number.");
    const long long v = std::strtoll(s.c_str(), nullptr, 10);
    if (v < min || v > max)
        throw ValidationException(field + " must be between " + std::to_string(min) + " and " + std::to_string(max) + ".");
    return static_cast<int>(v);
}

DateTime InputValidator::dateTime(const std::string& raw) { return parseDateTime(trim(raw)); }

std::string InputValidator::username(const std::string& raw) {
    const std::string u = trim(raw);
    AuthService::validateUsername(u);
    return u;
}

std::string InputValidator::password(const std::string& raw) {
    AuthService::validatePassword(raw);   // not trimmed: spaces are legitimate password characters
    return raw;
}

}  // namespace lms
