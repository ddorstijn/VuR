#ifndef RENDERER_H
#define RENDERER_H

#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

/*
 * Structure for tracking information used / created / modified
 * by utility functions.
 */
typedef struct VulkanContext
{
    const char* name;   // Name to put on the window/icon
    GLFWwindow* window; // Window handle

    VkSurfaceKHR surface;
    // bool prepared;
    // bool use_staging_buffer;
    // bool save_images;

    // std::vector<const char*> instance_layer_names;
    // std::vector<const char*> instance_extension_names;
    // std::vector<layer_properties> instance_layer_properties;
    // std::vector<VkExtensionProperties> instance_extension_properties;
    VkInstance instance;

    // std::vector<const char*> device_extension_names;
    // std::vector<VkExtensionProperties> device_extension_properties;
    VkPhysicalDevice* gpus;
    uint32_t gpu_count;
    uint32_t gpu_index;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkPhysicalDeviceProperties gpu_props;
    // std::vector<VkQueueFamilyProperties> queue_props;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkFramebuffer* framebuffers;
    int width, height;
    VkFormat format;

    uint32_t swapchainImageCount;
    VkSwapchainKHR swap_chain;
    // std::vector<swap_chain_buffer> buffers;
    VkSemaphore imageAcquiredSemaphore;

    VkCommandPool cmd_pool;

    struct
    {
        VkFormat format;

        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    // std::vector<struct texture_object> textures;

    struct
    {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    } uniform_data;

    struct
    {
        VkDescriptorImageInfo image_info;
    } texture_data;

    struct
    {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    } vertex_buffer;
    VkVertexInputBindingDescription vi_binding;
    VkVertexInputAttributeDescription vi_attribs[2];

    // glm::mat4 Projection;
    // glm::mat4 View;
    // glm::mat4 Model;
    // glm::mat4 Clip;
    // glm::mat4 MVP;

    VkCommandBuffer cmd; // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    // std::vector<VkDescriptorSetLayout> desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    VkPipelineShaderStageCreateInfo shaderStages[2];

    VkDescriptorPool desc_pool;
    // std::vector<VkDescriptorSet> desc_set;

    PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;
    PFN_vkDebugReportMessageEXT dbgBreakCallback;
    // std::vector<VkDebugReportCallbackEXT> debug_report_callbacks;

    uint32_t current_buffer;
    uint32_t queue_family_count;

    VkViewport viewport;
    VkRect2D scissor;
} VulkanContext;

VulkanContext*
vur_init_vulkan(const char* app_name);

void
vur_draw(VulkanContext* context);

void
vur_destroy(VulkanContext* context);

#endif // RENDERER_H