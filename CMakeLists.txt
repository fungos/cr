#
# project: cr
#
cmake_minimum_required(VERSION 3.21)
project(cr)

if (PROJECT_IS_TOP_LEVEL)
  set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
  set(FIPS_DYNAMIC_CRT 1)
  get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
  include("${FIPS_ROOT_DIR}/cmake/fips.cmake")
  fips_setup(PROJECT cr)
    add_definitions(-DCR_DEPLOY_PATH="${FIPS_PROJECT_DEPLOY_DIR}")
    include_directories(cr)
    add_subdirectory(samples)
    add_subdirectory(tests)
  fips_finish()
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
else()
  add_library(cr INTERFACE)
  target_include_directories(cr INTERFACE cr)
endif()

