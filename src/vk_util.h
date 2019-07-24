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
vutil_init_window(const char* app_name, GLFWwindow** window);

VuResult
vutil_init_instance(const char* app_name, VkInstance* instance);

VuResult
vutil_create_device(VkPhysicalDevice gpu, uint32_t graphics_queue_family_index, VkDevice* device);

VuResult
vutil_get_graphics_queue_family_index(VkPhysicalDevice gpu, uint32_t* graphics_queue_family_index);

VuResult
vutil_get_physical_devices(VkInstance instance, uint32_t* physical_device_count,
                           VkPhysicalDevice** physical_devices);

VuResult
vutil_select_suitable_physical_device(VkPhysicalDevice* physical_devices, uint32_t physical_device_count,
                                      uint32_t* physical_device_index);

VuResult
vutil_init_swapchain(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window,
                     VkSwapchainKHR* swapchain);

VuResult
vutil_init_command_buffer();

uint32_t
memory_type_from_properties(VkPhysicalDeviceMemoryProperties memory_properties, uint32_t typeBits,
                            VkFlags requirements_mask, uint32_t* typeIndex);

#endif // HELPER_H