#ifndef RENDERER_H
#define RENDERER_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

// TODO: Fix
#include "../extern/cglm/include/cglm/cglm.h"

#define FRAME_LAG 2
#define TEXTURE_COUNT 1

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
    VkCommandBuffer cmd;
    VkCommandBuffer graphics_to_present_cmd;
    VkImageView view;
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    VkFramebuffer framebuffer;
    VkDescriptorSet descriptor_set;
} SwapchainImageResources;

typedef struct
{
    VkSurfaceKHR surface;
    bool prepared;
    bool use_staging_buffer;
    bool separate_present_queue;
    bool is_minimized;

    GLFWwindow* window;
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
    VkSemaphore image_ownership_semaphores[FRAME_LAG];

    int width, height;
    VkFormat format;
    VkColorSpaceKHR color_space;

    uint32_t swapchain_image_count;
    VkSwapchainKHR swapchain;
    SwapchainImageResources* swapchain_image_resources;
    VkPresentModeKHR present_mode;
    VkFence fences[FRAME_LAG];
    int frame_index;

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

    struct texture_object textures[TEXTURE_COUNT];
    struct texture_object staging_texture;

    VkCommandBuffer command_buffer; // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout descriptor_layout;
    VkPipelineCache pipeline_cache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    mat4 projection;
    mat4 view;
    mat4 model;

    bool pause;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool descriptor_pool;

    uint32_t current_buffer;
    uint32_t queue_family_count;
} VulkanContext;

void
vur_init_vulkan(VulkanContext* ctx, const char* app_name);

void
vur_draw(VulkanContext* ctx);

void
vur_destroy(VulkanContext* ctx);

#endif // RENDERER_H