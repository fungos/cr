add_compile_definitions(CR_DEPLOY_PATH="${CMAKE_CURRENT_BINARY_DIR}")

add_library(basic_guest MODULE basic_guest.c)
target_link_libraries(basic_guest cr)

add_library(basic_guest_b MODULE basic_guest_b.c)
target_link_libraries(basic_guest_b cr)

add_executable(basic_host basic_host.cpp)
if(NOT WIN32)
    target_link_libraries(basic_host PRIVATE dl)
endif()
target_link_libraries(basic_host PUBLIC cr)

add_executable(basic_host_b basic_host_b.cpp)
if(NOT WIN32)
    target_link_libraries(basic_host_b PRIVATE dl)
endif()
target_link_libraries(basic_host_b PUBLIC cr)
