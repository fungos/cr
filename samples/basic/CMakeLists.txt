add_library(basic_guest SHARED basic_guest.c)
target_link_libraries(basic_guest cr)

add_executable(basic_host basic_host.cpp)
if(NOT WIN32)
    target_link_libraries(basic_host, dl)
endif()
target_link_libraries(basic_host cr)

add_library(basic_guest_b SHARED basic_guest_b.c)
target_link_libraries(basic_guest_b cr)

add_executable(basic_host_b basic_host_b.cpp)
if(NOT WIN32)
    target_link_libraries(basic_host_b, dl)
endif()
target_link_libraries(basic_host_b cr)
