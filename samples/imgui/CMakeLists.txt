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
  DOWNLOAD_ONLY
)
if (imgui_ADDED)
  # Based on https://github.com/fips-libs/fips-imgui/blob/1a38213f510f17a9bfd2026684b305670a216785/CMakeLists.txt
  add_library(imgui STATIC)
  target_include_directories(imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR})
  target_sources(imgui PRIVATE
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
  )
  if (MSVC)
    target_compile_options(imgui PRIVATE /W0)
  else()
    target_compile_options(imgui PRIVATE -w)
  endif()
  set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

include(cmake/imgui_deps.cmake)

add_subdirectory(GL)

add_library(imgui_guest MODULE imgui_guest.cpp)
target_include_directories(imgui_guest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_imgui_sample_deps(imgui_guest)

add_executable(imgui_host imgui_host.cpp)
target_include_directories(imgui_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_imgui_sample_deps(imgui_host)
target_compile_definitions(imgui_host PRIVATE CR_DEPLOY_PATH="${CMAKE_CURRENT_BINARY_DIR}")

if (WIN32)
  add_custom_command(TARGET imgui_host POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:imgui_host> $<TARGET_FILE_DIR:imgui_host>
    COMMAND_EXPAND_LISTS
  )
endif()