#
# project: cr
#
cmake_minimum_required(VERSION 3.0)
project(cr)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
SET(FIPS_DYNAMIC_CRT 1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT CMAKE_EXPORT_FIND_PACKAGE_NAME)
  get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
  include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

  if (NOT FIPS_IMPORT)
    fips_setup(PROJECT cr)
    add_definitions(-DCR_DEPLOY_PATH="${FIPS_PROJECT_DEPLOY_DIR}")
    add_subdirectory(samples)
    add_subdirectory(tests)
  endif()
endif()
