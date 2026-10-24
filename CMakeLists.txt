cmake_minimum_required(VERSION 3.16)
project(ModeratedLMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

# Everything except main() is a library, so the unit tests can link against it.
add_library(lms_core
  src/domain/Types.cpp
  src/domain/IClock.cpp
  src/domain/User.cpp
  src/domain/UserFactory.cpp
  src/domain/Submission.cpp
  src/domain/LatePenaltyPolicy.cpp
  src/data/Database.cpp
  src/data/SqliteRepositories.cpp
  src/service/PasswordHasher.cpp
  src/service/AuthService.cpp
  src/service/AuditService.cpp
  src/service/CourseService.cpp
  src/service/AssignmentService.cpp
  src/service/SubmissionService.cpp
  src/service/ModerationService.cpp
  src/ui/Console.cpp
  src/ui/InputValidator.cpp
  src/ui/Menu.cpp)
target_include_directories(lms_core PUBLIC src)
target_link_libraries(lms_core PUBLIC SQLite::SQLite3 PkgConfig::SODIUM)

add_executable(lms src/main.cpp)
target_link_libraries(lms PRIVATE lms_core)
