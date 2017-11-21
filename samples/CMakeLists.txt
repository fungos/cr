fips_begin_sharedlib(basic_guest cmdline)
    fips_files(basic_guest.cpp)
fips_end_sharedlib()

fips_begin_app(basic_host cmdline)
    fips_files(basic_host.cpp)
    if (FIPS_LINUX OR FIPS_MACOS)
        fips_libs(dl stdc++fs)
    endif()
fips_end_app()

fips_begin_sharedlib(imgui_guest cmdline)
    fips_files(imgui_guest.cpp)
    fips_src(GL GROUP_FOLDERS)
    include_directories(.)

    fips_deps(imgui glfw3)

    if (FIPS_WINDOWS)
        fips_libs(opengl32)
    endif()

    if (FIPS_LINUX)
        fips_libs(X11 Xrandr Xi Xinerama Xxf86vm Xcursor GL m dl)
    endif()

    if (FIPS_MACOS)
        fips_frameworks_osx(Cocoa CoreVideo OpenGL)
        fips_libs(m dl)
    endif()
fips_end_sharedlib()

fips_begin_app(imgui_host cmdline)
    fips_files(imgui_host.cpp)
    fips_src(GL GROUP_FOLDERS)
    include_directories(.)
    fips_deps(imgui glfw3)

    if (FIPS_WINDOWS)
        fips_libs(opengl32)
    endif()

    if (FIPS_LINUX)
        fips_libs(X11 Xrandr Xi Xinerama Xxf86vm Xcursor GL m dl stdc++fs)
    endif()

    if (FIPS_MACOS)
        fips_frameworks_osx(Cocoa CoreVideo OpenGL)
        fips_libs(m dl stdc++fs)
    endif()
fips_end_app()


if (FIPS_MSVC)
    # vs2017 filesystem support
    set_target_properties(basic_guest PROPERTIES COMPILE_FLAGS "/std:c++latest")
    set_target_properties(basic_host  PROPERTIES COMPILE_FLAGS "/std:c++latest")
    set_target_properties(imgui_guest PROPERTIES COMPILE_FLAGS "/std:c++latest")
    set_target_properties(imgui_host  PROPERTIES COMPILE_FLAGS "/std:c++latest")
endif()
