// Console abstraction so the menus can be driven by a script in tests.
#pragma once
#include <iostream>
#include <string>

namespace lms {

class Console {
public:
    virtual ~Console() = default;
    virtual void println(const std::string& text) = 0;
    // Throws InputClosedException at end of input.
    virtual std::string readLine(const std::string& prompt) = 0;
    virtual std::string readSecret(const std::string& prompt) = 0;   // no echo where supported
};

class StdConsole : public Console {
public:
    void println(const std::string& text) override;
    std::string readLine(const std::string& prompt) override;
    std::string readSecret(const std::string& prompt) override;
};

}  // namespace lms
