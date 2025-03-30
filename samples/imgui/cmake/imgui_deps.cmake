function(set_imgui_deps target)
    target_link_libraries(${target} PRIVATE glfw3 imgui cr)
if(WIN32)
    target_link_libraries(${target} PRIVATE opengl32)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Untested - I don't have a mac to test on
    # Direct port of prior fips code
    target_link_libraries(${target} PRIVATE "-framework Cocoa -framework CoreVideo -framework OpenGL")
    target_link_libraries(${target} PRIVATE m dl)
else()
    target_link_libraries(${target} PRIVATE X11 Xrandr Xi Xinerama Xxf86vm Xcursor GL m dl)
endif()
endfunction()