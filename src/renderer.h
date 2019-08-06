#ifndef RENDERER_H
#define RENDERER_H

#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

// TODO: Fix
#include "../extern/cglm/include/cglm/cglm.h"

/*
 * Structure for tracking information used / created / modified
 * by utility functions.
 */
typedef struct VulkanContext
{
    const char* name;   // Name to put on the window/icon
    GLFWwindow* window; // Window handle

    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice* gpus;
    uint32_t gpu_count;
    uint32_t gpu_index;

    VkDevice device;
    VkQueue graphics_queue;

    /** @brief Contains queue family indices */
    struct
    {
        uint32_t graphics;
        uint32_t compute;
        uint32_t transfer;
    } queueFamilyIndices;

    VkFramebuffer* framebuffers;
    VkFormat format;

    uint32_t swapchain_image_count;
    VkSwapchainKHR swapchain;

    VkSemaphore present_complete;
    VkSemaphore render_complete;
    VkSubmitInfo submit_info;

    VkCommandPool command_pool;

    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 clip;
    mat4 mvp;
} VulkanContext;

VulkanContext*
vur_init_vulkan(const char* app_name);

void
vur_draw(VulkanContext* context);

void
vur_destroy(VulkanContext* context);

#endif // RENDERER_H