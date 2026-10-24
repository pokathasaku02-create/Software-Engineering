// Exception hierarchy. Every error the application signals derives from LmsException,
// so the menus can catch one type and keep the program running.
#pragma once
#include <stdexcept>
#include <string>

namespace lms {

class LmsException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ValidationException : public LmsException { public: using LmsException::LmsException; };
class AuthenticationException : public LmsException { public: using LmsException::LmsException; };
class AccessDeniedException : public LmsException { public: using LmsException::LmsException; };
class NotFoundException : public LmsException { public: using LmsException::LmsException; };
class IllegalStateException : public LmsException { public: using LmsException::LmsException; };
class ResubmissionClosedException : public LmsException { public: using LmsException::LmsException; };
class DatabaseException : public LmsException { public: using LmsException::LmsException; };
class InputClosedException : public LmsException { public: using LmsException::LmsException; };

}  // namespace lms
