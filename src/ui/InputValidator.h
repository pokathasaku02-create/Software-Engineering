// Presentation-layer component that checks and sanitises ALL user input before any service sees it.
#pragma once
#include <string>

#include "../domain/Types.h"

namespace lms {

class InputValidator {
public:
    // Trims whitespace, removes control characters, and enforces length limits.
    static std::string text(const std::string& raw, const std::string& field, std::size_t minLen, std::size_t maxLen);
    static int integer(const std::string& raw, const std::string& field, int min, int max);
    static DateTime dateTime(const std::string& raw);
    static std::string username(const std::string& raw);
    static std::string password(const std::string& raw);
};

}  // namespace lms
