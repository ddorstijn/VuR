/**
 * @file vk_util.h
 * @author Danny Dorstijn (dannydorstijn1997@gmail.com)
 * @brief Vulkan Utility Functions. Initializers and tools
 * @version 0.8
 * @date 2019-08-27
 *
 * @copyright Copyright (c) 2019
 *
 */

#ifndef HELPER_H
#define HELPER_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <stdbool.h>

/**
 * @brief Create window and check support
 *
 * @param[in] app_name The name of the application for window name
 * @param[out] window The window handle for GLFW
 * @return true Creation was succesfull
 * @return false Creation was unsuccesfull
 */
bool
vut_init_window(const char app_name[], GLFWwindow** window);

/**
 * @brief Initialize a new vulkan instance
 *
 * @param[in] app_name The name of the application
 * @param[out] instance The pointer that will point to the created instance
 * @return VkResult Result of the vkCreateInstance function
 */
VkResult
vut_init_instance(const char app_name[], VkInstance* instance);

/**
 * @brief Get a list of all gpu's. If NULL is passed the device count will be filled
 *
 * @param[in] instance Instance handle required by vulkan
 * @param[in] physical_device_count The amount of devices in the system
 * @param[out] physical_devices The list of devices
 * @return VkResult The result of the Enumerate Devices function
 */
VkResult
vut_get_physical_devices(VkInstance instance,
                         uint32_t* physical_device_count,
                         VkPhysicalDevice physical_devices[]);

/**
 * @brief Select the best fitting GPU from the list
 *
 * @param[in] gpus List of physical devices
 * @param[in] gpu_count The size of the list
 * @param[out] gpu The selected GPU
 * @return VkResult VK_SUCCESS if all is fine
 */
VkResult
vut_pick_physical_device(VkPhysicalDevice* gpus, uint32_t gpu_count, VkPhysicalDevice gpu[]);

/**
 * @brief Initialize the Vulkan device
 *
 * @param[in] gpu The GPU the device is created for
 * @param[in] graphics_queue_family_index The queue for graphics presentation
 * @param[out] device The created device
 * @return VkResult VK_SUCCESS if device is created succesfully
 */
VkResult
vut_init_device(VkPhysicalDevice gpu, uint32_t graphics_queue_family_index, VkDevice* device);

/**
 * @brief Get the indices of the graphics and present queues
 *
 * @param[in] gpu The handle ffor the vulkan physical device
 * @param[in] surface The surface handle
 * @param[out] graphics_queue_family_index
 * @param[out] present_queue_family_index
 * @param[out] separate_present_queue
 * @return VkResult VK_SUCCESS if all is fine
 */
VkResult
vut_get_queue_family_indices(VkPhysicalDevice gpu,
                             VkSurfaceKHR surface,
                             uint32_t* graphics_queue_family_index,
                             uint32_t* present_queue_family_index,
                             bool* separate_present_queue);

/**
 * @brief Create a surface for the framebuffer
 *
 * @param[in] instance The vulkan instance handle
 * @param[in] window The GLFW window handle
 * @param[out] surface The created surface
 * @return VkResult VK_SUCCESS if surface created succefully
 */
VkResult
vut_init_surface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR* surface);

/**
 * @brief Select the most preferable present mode
 *
 * @param[in] capabilities Capabilities of the GPU, get them with
 * vkGetPhysicalDeviceSurfaceCapabilitiesKHR
 * @param[in] gpu Physical device handle
 * @param[in] surface Surface handle
 * @return VkPresentModeKHR Most preferable present mode
 */
VkPresentModeKHR
vut_get_present_mode(VkSurfaceCapabilitiesKHR capabilities,
                     VkPhysicalDevice gpu,
                     VkSurfaceKHR surface);

/**
 * @brief Check window size with capabilities
 *
 * @param[in] capabilities Capabilities of the GPU, get them with
 * @param[in] window_extent The window size
 * @return VkExtent2D The chosen extent
 */
VkExtent2D
vut_get_swapchain_extent(VkSurfaceCapabilitiesKHR capabilities, VkExtent2D window_extent);

/**
 * @brief Get the most preferable surface format
 *
 * @param[in] gpu Physical device handle
 * @param[in] surface Surface handle
 * @param[out] format The most preferable surface format
 * @param[out] color_space The colorspace that corresponds with the chosen format
 * @return VkResult Return value of vkGetPhysicalDeviceFormats
 */
VkResult
vut_get_surface_format(VkPhysicalDevice gpu,
                       VkSurfaceKHR surface,
                       VkFormat* format,
                       VkColorSpaceKHR* color_space);

/**
 * @brief Initializer for the swapchain
 *
 * @param[in] gpu Physical device handle
 * @param[in] device Vulkan device handle
 * @param[in] surface Surface handle
 * @param[in] capabilities Surface capabilities, needed for image_count and preTransform
 * @param[in] extent The swapchain extent
 * @param[in] format The chosen surface format
 * @param[in] present_mode The chosen present mode
 * @param[in] color_space The chosen color space
 * @param[in, out] swapchain Old swapchain will be destroyed. Points to newly created swapchain
 * @return VkResult The result of vkCreateSwapchainKHR or vkDestroySwapchainKHR
 */
VkResult
vut_init_swapchain(VkPhysicalDevice gpu,
                   VkDevice device,
                   VkSurfaceKHR surface,
                   VkSurfaceCapabilitiesKHR capabilities,
                   VkExtent2D extent,
                   VkFormat format,
                   VkPresentModeKHR present_mode,
                   VkColorSpaceKHR color_space,
                   VkSwapchainKHR* swapchain);

/**
 * @brief Initialize image for Vulkan
 *
 * @param[in] device Vulkan device handle
 * @param[in] format The surface format
 * @param[in] window_extent The window size
 * @param[out] image The create image handle
 * @return VkResult The result of vkCreateImage
 */
VkResult
vut_init_image(VkDevice device, VkFormat format, VkExtent2D window_extent, VkImage* image);

/**
 * @brief
 *
 * @param[in] device Vulkan device handle
 * @param[in] format The surface format
 * @param[in] swapchain_image
 * @param[out] image_view
 * @return VkResult
 */
VkResult
vut_init_image_view(VkDevice device,
                    VkFormat format,
                    VkImage swapchain_image,
                    VkImageView* image_view);

/**
 * @brief Initialize a shader for Vulkan
 *
 * @param[in] device Device handle
 * @param[in] shader_name Path + name of the shader
 * @param[out] shader_module The created module
 * @return VkResult The return of vkCreateShaderModule
 */
VkResult
vut_init_shader_module(VkDevice device, char* shader_name, VkShaderModule* shader_module);

/**
 * @brief Initialize the layout for the grpahics pipeline
 *
 * @param[in] device The vulkan device handle
 * @param[in] descriptor_layout Layout of the descriptor set
 * @param[out] pipeline_layout The created pipeline layout
 * @return VkResult
 */
VkResult
vut_init_pipeline_layout(VkDevice device,
                         VkDescriptorSetLayout* descriptor_layout,
                         VkPipelineLayout* pipeline_layout);

/**
 * @brief Create the pipeline
 *
 * @param[in] device
 * @param[in] stages
 * @param[in] vertex_input
 * @param[in] input_assembly
 * @param[in] viewport_state
 * @param[in] rasterizer
 * @param[in] multisampling
 * @param[in] color_blending
 * @param[in] pipeline_layout
 * @param[in] render_pass
 * @param[out] pipeline
 * @return VkResult
 */
VkResult
vut_init_pipeline(VkDevice device,
                  const VkPipelineShaderStageCreateInfo stages[],
                  const VkPipelineVertexInputStateCreateInfo* vertex_input,
                  const VkPipelineInputAssemblyStateCreateInfo* input_assembly,
                  const VkPipelineViewportStateCreateInfo* viewport_state,
                  const VkPipelineRasterizationStateCreateInfo* rasterizer,
                  const VkPipelineMultisampleStateCreateInfo* multisampling,
                  const VkPipelineColorBlendStateCreateInfo* color_blending,
                  VkPipelineLayout pipeline_layout,
                  VkRenderPass render_pass,
                  VkPipeline* pipeline);

/**
 * @brief Create the render pass
 *
 * @param[in] device The Vulkan device handle
 * @param[in] surface_format The surface format
 * @param[out] render_pass The created render pass
 * @return VkResult
 */
VkResult
vut_prepare_render_pass(VkDevice device, VkFormat surface_format, VkRenderPass* render_pass);

/**
 * @brief Create a framebuffer
 *
 * @param[in] device The Vulkan device handle
 * @param[in] render_pass The render pass handle
 * @param[in] image_view The image view the framebuffer writes to
 * @param[in] window_extent The window size
 * @param[out] framebuffer The created framebuffer
 * @return VkResult
 */
VkResult
vut_prepare_framebuffer(VkDevice device,
                        VkRenderPass render_pass,
                        VkImageView image_view,
                        VkExtent2D window_extent,
                        VkFramebuffer* framebuffer);

/**
 * @brief Create a semaphore for synchronisation on the gpu
 *
 * @param[in] device The Vulkan device handle
 * @param[out] semaphore The created semaphore
 * @return VkResult
 */
VkResult
vut_init_semaphore(VkDevice device, VkSemaphore* semaphore);

/**
 * @brief Create a new fence for synchronisation
 *
 * @param[in] device The Vulkan device handle
 * @param[out] fence The created fence
 * @return VkResult
 */
VkResult
vut_init_fence(VkDevice device, VkFence* fence);

/**
 * @brief Create a new command pool
 *
 * @param[in] device The Vulkan device handle
 * @param[in] family_index One command pool can only apply to one queue
 * @param[out] command_pool The created command pool
 * @return VkResult
 */
VkResult
vut_init_command_pool(VkDevice device, uint32_t family_index, VkCommandPool* command_pool);

/**
 * @brief Allocate the command buffer
 *
 * @param[in] device The Vulkan device handle
 * @param[in] command_pool
 * @param[in] count
 * @param[out] command_buffer
 * @return VkResult
 */
VkResult
vut_alloc_command_buffer(VkDevice device,
                         VkCommandPool command_pool,
                         uint32_t count,
                         VkCommandBuffer* command_buffer);

/**
 * @brief Start recording with the command buffer
 *
 * @param[in] command_buffer The buffer that should start recording
 * @return VkResult
 */
VkResult
vut_begin_command_buffer(VkCommandBuffer command_buffer);

/**
 * @brief Start the render pass
 *
 * @param[in] buffer The command buffer the render pass should record to
 * @param[in] render_pass The render pass handle
 * @param[in] framebuffer The framebuffer where the drawing will happen
 * @param[in] extent The extent of the swapchain
 * @return VkResult
 */
VkResult
vut_begin_render_pass(VkCommandBuffer buffer,
                      VkRenderPass render_pass,
                      VkFramebuffer framebuffer,
                      VkExtent2D extent);

#endif // HELPER_H