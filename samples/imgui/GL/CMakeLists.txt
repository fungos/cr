add_library(gl3w STATIC gl3w.c)
target_include_directories(gl3w SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
set_target_properties(gl3w PROPERTIES POSITION_INDEPENDENT_CODE ON)