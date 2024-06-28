#
# project: cr
#

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
SET(FIPS_DYNAMIC_CRT 1)

cmake_minimum_required(VERSION 3.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

if (NOT FIPS_IMPORT)
    fips_setup(PROJECT cr)
        add_definitions(-DCR_DEPLOY_PATH="${FIPS_PROJECT_DEPLOY_DIR}")
        add_subdirectory(samples)
        add_subdirectory(tests)
    fips_finish()
endif()
