#include "Console.h"

#include "../domain/Exceptions.h"

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

namespace lms {

void StdConsole::println(const std::string& text) { std::cout << text << '\n'; }

std::string StdConsole::readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosedException("Input closed.");
    return line;
}

std::string StdConsole::readSecret(const std::string& prompt) {
#if defined(__unix__) || defined(__APPLE__)
    termios oldt{};
    const bool tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldt) == 0;
    if (tty) {
        termios newt = oldt;
        newt.c_lflag &= static_cast<tcflag_t>(~ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
    std::string line;
    try {
        line = readLine(prompt);
    } catch (...) {
        if (tty) tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        throw;
    }
    if (tty) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << '\n';
    }
    return line;
#else
    return readLine(prompt);   // Windows: input is echoed (documented limitation)
#endif
}

}  // namespace lms
