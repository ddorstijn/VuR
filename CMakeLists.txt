cmake_minimum_required(VERSION 3.13)
project(VUSE VERSION 0.01 LANGUAGES C)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_subdirectory(src)
add_subdirectory(app)

# Set directory for cmake helpers
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message("debug mode")
ENDIF()