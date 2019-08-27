#ifndef RENDERER_H
#define RENDERER_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

// TODO: Fix
#include "../extern/cglm/include/cglm/cglm.h"

#define FRAME_LAG 2

/*
 * structure to track all objects related to a texture.
 */
struct texture_object
{
    VkSampler sampler;

    VkImage image;
    VkBuffer buffer;
    VkImageLayout imageLayout;

    VkMemoryAllocateInfo mem_alloc;
    VkDeviceMemory mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

/**
 * @brief Structure for tracking information used / created / modified
 * by utility functions.
 */
typedef struct
{
    VkImage image;
    VkCommandBuffer command_buffer;
    VkImageView view;
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    VkFramebuffer framebuffer;
    VkDescriptorSet descriptor_set;
} SwapchainImageResources;

/**
 * @brief Context for the renderer
 *
 */
typedef struct _VulkanContext
{
    bool separate_present_queue;

    GLFWwindow* window;
    VkExtent2D window_extent;
    const char* name;

    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkSemaphore image_acquired_semaphores[FRAME_LAG];
    VkSemaphore draw_complete_semaphores[FRAME_LAG];

    VkFormat surface_format;
    VkColorSpaceKHR color_space;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    uint32_t swapchain_image_count;
    SwapchainImageResources* swapchain_image_resources;
    VkPresentModeKHR present_mode;
    VkFence fences[FRAME_LAG];

    VkCommandPool command_pool;
    VkCommandPool present_command_pool;

    struct
    {
        VkFormat format;

        VkImage image;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    VkDescriptorSetLayout descriptor_layout;
    VkDescriptorPool descriptor_pool;

    VkRenderPass render_pass;

    mat4 projection;
    mat4 view;
    mat4 model;

    bool should_quit;
    bool framebuffer_resized;

    uint32_t current_buffer;
    int frame_index;
} VulkanContext;

// Init
/**
 * @brief Wrapper to initialize renderer
 *
 * @param[in] ctx VulkanContext handle
 * @param app_name Name off the application
 */
void
vur_init(VulkanContext* ctx, const char* app_name);

// Main loop
/**
 * @brief Get events from window
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_update_window(VulkanContext* ctx);

/**
 * @brief Draw new frame
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_draw(VulkanContext* ctx);

// Destroy
/**
 * @brief Destroy the renderer before closing app
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_destroy(VulkanContext* ctx);

#endif // RENDERER_H