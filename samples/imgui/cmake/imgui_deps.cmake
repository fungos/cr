function(set_imgui_sample_deps target)
    target_link_libraries(${target} PRIVATE imgui GL gl3w glfw cr)
if(WIN32)
    target_link_libraries(${target} PRIVATE opengl32)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Untested - I don't have a mac to test on
    # Direct port of prior fips code
    target_link_libraries(${target} PRIVATE "-framework Cocoa -framework CoreVideo -framework OpenGL")
    target_link_libraries(${target} PRIVATE m dl)
else()
    target_link_libraries(${target} PRIVATE X11 Xrandr Xi Xinerama Xxf86vm Xcursor m dl)
endif()
endfunction()