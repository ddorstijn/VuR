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

/*
 * Structure for tracking information used / created / modified
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
void
vur_init(VulkanContext* ctx, const char* app_name);

// Main loop
void
vur_update_window(VulkanContext* ctx);

void
vur_draw(VulkanContext* ctx);

// Destroy
void
vur_destroy(VulkanContext* ctx);

#endif // RENDERER_H