cmake_minimum_required(VERSION 3.10)
project(VUSE VERSION 0.01 LANGUAGES C)

find_package(Vulkan REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw3)

# GLM
add_subdirectory(extern/cglm)

add_subdirectory(src)
add_subdirectory(app)

# Set directory for cmake helpers
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})