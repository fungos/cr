# Based on: https://github.com/adrianderstroff/opengl-starter/blob/master/cmake/Packages.cmake#L3
cpmaddpackage(
  NAME glfw
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.3.8
  OPTIONS "BUILD_SHARED_LIBS ON"
          "GLFW_BUILD_EXAMPLES OFF"
          "GLFW_BUILD_TESTS OFF"
          "GLFW_BUILD_DOCS OFF"
)

# Works simply just like this:
cpmaddpackage(
  NAME imgui
  GITHUB_REPOSITORY ocornut/imgui
  VERSION 1.82
)

include(cmake/imgui_deps.cmake)

add_library(imgui_guest MODULE imgui_guest.cpp GL/gfl3w.c)
target_include_directories(imgui_guest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_imgui_deps(imgui_guest)


add_executable(imgui_host imgui_host.cpp GL/gl3w.c)
target_include_directories(imgui_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui_host PRIVATE glfw3 imgui)
set_imgui_deps(imgui_host)
