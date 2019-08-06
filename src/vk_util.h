#ifndef HELPER_H
#define HELPER_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

typedef enum VuResult
{
    VUR_SUCCESS = 0,
    VUR_ALLOC_FAILED = 1,
    VUR_GLFW_INIT_FAILED = 2,
    VUR_VULKAN_NOT_SUPPORTED = 3,
    VUR_WINDOW_CREATION_FAILED = 4,
    VUR_INSTANCE_CREATION_FAILED = 5,
    VUR_DEVICE_CREATION_FAILED = 6,
    VUR_PHYSICIAL_DEVICE_CREATION_FAILED = 7,
} VuResult;

VuResult
vut_init_window(const char* app_name, GLFWwindow** window);

VuResult
vut_init_instance(const char* app_name, VkInstance* instance);

VuResult
vut_create_device(VkPhysicalDevice gpu, uint32_t graphics_queue_family_index, VkDevice* device);

VuResult
vut_get_graphics_queue_family_index(VkPhysicalDevice gpu, uint32_t* graphics_queue_family_index);

VuResult
vut_get_physical_devices(VkInstance instance, uint32_t* physical_device_count,
                         VkPhysicalDevice** physical_devices);

VuResult
vut_pick_physical_device(VkPhysicalDevice* physical_devices, uint32_t physical_device_count,
                         uint32_t* physical_device_index);

VuResult
vut_init_swapchain(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window,
                   VkSwapchainKHR* swapchain, VkFormat* format, VkColorSpaceKHR* color_space);

VuResult
vut_init_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, VkFormat format, uint32_t* image_count);

VuResult
vut_create_semaphore(VkDevice device, VkSemaphore* semaphore);

VkSubmitInfo
vut_create_submit_info(VkSemaphore* present_complete, VkSemaphore* render_complete);

VuResult
vut_create_command_pool(VkDevice device, uint32_t family_index, VkCommandPool* command_pool);

#endif // HELPER_H