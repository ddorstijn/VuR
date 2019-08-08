#ifndef HELPER_H
#define HELPER_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <stdbool.h>

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
vut_get_queue_family_indices(VkPhysicalDevice gpu,
                             VkSurfaceKHR surface,
                             uint32_t* queue_family_count,
                             uint32_t* graphics_queue_family_index,
                             uint32_t* present_queue_family_index,
                             bool* separate_present_queue);

VuResult
vut_get_physical_devices(VkInstance instance,
                         uint32_t* physical_device_count,
                         VkPhysicalDevice** physical_devices);

VuResult
vut_pick_physical_device(VkPhysicalDevice* gpus, uint32_t gpu_count, VkPhysicalDevice* gpu);

VuResult
vut_init_swapchain(VkPhysicalDevice gpu,
                   VkDevice device,
                   VkSurfaceKHR surface,
                   GLFWwindow* window,
                   VkSwapchainKHR* swapchain,
                   VkFormat* format,
                   VkColorSpaceKHR* color_space);

VuResult
vut_init_swapchain_images(VkDevice device,
                          VkSwapchainKHR swapchain,
                          VkFormat format,
                          uint32_t* image_count);

VuResult
vut_create_semaphore(VkDevice device, VkSemaphore* semaphore);

VuResult
vut_create_fence(VkDevice device, VkFence* fence);

VkSubmitInfo
vut_create_submit_info(VkSemaphore* present_complete, VkSemaphore* render_complete);

VuResult
vut_create_command_pool(VkDevice device, uint32_t family_index, VkCommandPool* command_pool);

VuResult
vut_alloc_command_buffer(VkDevice device,
                         VkCommandPool command_pool,
                         VkCommandBuffer* command_buffer);

VuResult
vut_begin_command_buffer(VkCommandBuffer command_buffer);

#endif // HELPER_H