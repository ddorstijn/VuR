add_library(
    vulkan_renderer
    renderer.c
    renderer.h
    internal.h
    vk_util.c
    vk_util.h
)

target_include_directories(vulkan_renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkan_renderer PUBLIC glfw)
target_link_libraries(vulkan_renderer PUBLIC Vulkan::Vulkan)
target_link_libraries(vulkan_renderer PUBLIC cglm)

target_compile_definitions(vulkan_renderer PRIVATE VK_USE_PLATFORM_WIN32_KHR)
