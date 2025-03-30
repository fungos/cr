# Based on: https://github.com/cpm-cmake/CPM.cmake/blob/master/examples/gtest/CMakeLists.txt#L11
cpmaddpackage(
  NAME googletest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG release-1.10.0
  VERSION 1.10.0
  OPTIONS
  "INSTALL_GTEST OFF"
  "gtest_force_shared_crt"
  "BUILD_GMOCK OFF"
  "BUILD_GTEST ON"
  SYSTEM
)
target_compile_options(gtest PRIVATE -Wno-error=maybe-uninitialized)

add_library(test_basic MODULE test_basic.cpp test_basic.x)
target_include_directories(test_basic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_basic cr)

add_executable(crTest test.cpp test_basic.x)
target_include_directories(crTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(crTest test_basic)
target_compile_definitions(cr INTERFACE CR_DEPLOY_PATH="${CMAKE__CURRENT_BINARY_DIR}")
target_compile_features(crTest PRIVATE cxx_std_17)

if (NOT WIN32)
    target_link_libraries(crTest PRIVATE dl)
endif()
if (NOT MSVC)
    set_target_properties(crTest PROPERTIES COMPILE_FLAGS "-fno-stack-protector")
endif()

target_link_libraries(crTest PRIVATE gtest gtest_main cr)
enable_testing()
add_test(crTest crTest)
