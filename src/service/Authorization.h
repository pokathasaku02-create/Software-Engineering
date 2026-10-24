// Small helpers every service uses to check who is acting. Business rules about WHO may do
// WHAT live in the services (not the menus), so a changed menu can never bypass them.
#pragma once
#include <string>

#include "../domain/Exceptions.h"
#include "../domain/User.h"

namespace lms {

inline void requirePermission(const User& actor, Permission p, const char* what) {
    if (!actor.isActive() || !actor.canPerform(p))
        throw AccessDeniedException(std::string("Your role is not allowed to ") + what + ".");
}

}  // namespace lms
