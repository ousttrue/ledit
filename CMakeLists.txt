cmake_minimum_required(VERSION 3.16)
project("ledit")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/bin)

set(CMAKE_CXX_STANDARD 17)
set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)

subdirs(glad)

add_executable(
  ledit
  src/glutil/vertex_buffer.cpp
  src/glutil/drawable.cpp
  src/glutil/texture.cpp
  src/glutil/shader.cpp
  src/glutil/gpu.cpp
  src/la.cc
  src/main.cc
  src/state.cc
  src/u8String.cc
  src/languages.cc
  src/utils.cpp
  src/glfwapp.cpp
  src/cursor.cpp
  src/font_atlas.cpp)
add_subdirectory(third-party/glfw)
add_subdirectory(third-party/freetype2)

if(NOT WIN32 AND NOT APPLE)
  target_link_libraries(ledit PRIVATE glad glfw freetype fontconfig dl)
else()
  target_link_libraries(ledit PRIVATE glad glfw freetype)
endif()

if(APPLE)
  # set(CMAKE_CXX_FLAGS_RELEASE "-o3")
endif()
