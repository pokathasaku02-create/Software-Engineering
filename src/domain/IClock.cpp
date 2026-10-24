#include "IClock.h"

#include <ctime>

namespace lms {

DateTime SystemClock::now() const {
    const std::time_t raw = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &raw);
#else
    localtime_r(&raw, &local);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &local);
    return parseDateTime(buf);
}

}  // namespace lms
