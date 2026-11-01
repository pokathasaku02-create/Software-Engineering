# GoogleTest: use an installed copy if there is one, otherwise download v1.14.0.
include(FetchContent)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(lms_tests
  test_domain.cpp
  test_repositories.cpp
  test_auth_course.cpp
  test_submission_moderation.cpp
  test_ui_system.cpp)
target_include_directories(lms_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lms_tests PRIVATE lms_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(lms_tests)
