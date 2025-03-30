#
# project: cr
#
cmake_minimum_required(VERSION 3.21...3.31)
project(cr)

if (PROJECT_IS_TOP_LEVEL)
  include(cmake/fetch_dependencies.cmake)
  add_subdirectory(cr)
  add_subdirectory(tests)
  add_subdirectory(samples)
else()
  add_subdirectory(cr)
endif()

