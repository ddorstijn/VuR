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
 * @param app_name The name of the application for window name
 * @param window The window handle for GLFW
 * @return true Creation was succesfull
 * @return false Creation was unsuccesfull
 */
bool
vut_init_window(const char app_name[], GLFWwindow** window);

/**
 * @brief Initialize a new vulkan instance
 *
 * @param app_name The name of the application
 * @param[out] instance The pointer that will point to the created instance
 * @return VkResult Result of the vkCreateInstance function
 */
VkResult
vut_init_instance(const char app_name[], VkInstance* instance);

/**
 * @brief Get a list of all gpu's. If NULL is passed the device count will be filled
 *
 * @param instance Instance handle required by vulkan
 * @param physical_device_count The amount of devices in the system
 * @param[out] physical_devices The list of devices
 * @return VkResult The result of the Enumerate Devices function
 */
VkResult
vut_get_physical_devices(VkInstance instance,
                         uint32_t* physical_device_count,
                         VkPhysicalDevice physical_devices[]);

VkResult
vut_pick_physical_device(VkPhysicalDevice* gpus, uint32_t gpu_count, VkPhysicalDevice gpu[]);

VkResult
vut_init_device(VkPhysicalDevice gpu, uint32_t graphics_queue_family_index, VkDevice* device);

VkResult
vut_get_queue_family_indices(VkPhysicalDevice gpu,
                             VkSurfaceKHR surface,
                             uint32_t* graphics_queue_family_index,
                             uint32_t* present_queue_family_index,
                             bool* separate_present_queue);

VkResult
vut_init_surface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR* surface);

VkPresentModeKHR
vut_get_present_mode(VkSurfaceCapabilitiesKHR capabilities,
                     VkPhysicalDevice gpu,
                     VkSurfaceKHR surface);

VkExtent2D
vut_get_swapchain_extent(VkSurfaceCapabilitiesKHR capabilities, VkExtent2D window_extent);

VkResult
vut_get_surface_format(VkPhysicalDevice gpu,
                       VkSurfaceKHR surface,
                       VkFormat* format,
                       VkColorSpaceKHR* color_space);

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

VkResult
vut_init_image(VkDevice device, VkFormat format, VkExtent2D window_extent, VkImage* image);

VkResult
vut_init_image_view(VkDevice device,
                    VkFormat format,
                    VkImage swapchain_image,
                    VkImageView* image_view,
                    bool is_depth);

VkResult
vut_init_shader_module(VkDevice device, char* shader_name, VkShaderModule* shader_module);

VkResult
vut_init_pipeline_layout(VkDevice device,
                         VkDescriptorSetLayout* descriptor_layout,
                         VkPipelineLayout* pipeline_layout);

VkResult
vut_prepare_render_pass(VkDevice device, VkFormat color_format, VkRenderPass* render_pass);

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

VkResult
vut_init_descriptor_layout(VkDevice device, VkDescriptorSetLayout* descriptor_layout);

VkResult
vut_init_descriptor_pool(VkDevice device, uint32_t image_count, VkDescriptorPool* descriptor_pool);

VkResult
vut_init_descriptor_set(VkDevice device,
                        VkDescriptorPool pool,
                        VkDescriptorSetLayout* layout,
                        uint32_t image_count);

VkResult
vut_prepare_framebuffer(VkDevice device,
                        VkRenderPass render_pass,
                        VkImageView image_view,
                        VkExtent2D window_extent,
                        VkFramebuffer* framebuffer);

VkResult
vut_init_semaphore(VkDevice device, VkSemaphore* semaphore);

VkResult
vut_init_fence(VkDevice device, VkFence* fence);

VkResult
vut_build_image_ownership_cmd(VkCommandBuffer present_buffer,
                              uint32_t graphics_queue_family_index,
                              uint32_t present_queue_family_index,
                              VkImage swapchain_image);

const VkSubmitInfo
vut_init_submit_info(VkSemaphore* present_complete, VkSemaphore* render_complete);

VkResult
vut_init_command_pool(VkDevice device, uint32_t family_index, VkCommandPool* command_pool);

VkResult
vut_alloc_command_buffer(VkDevice device,
                         VkCommandPool command_pool,
                         uint32_t count,
                         VkCommandBuffer* command_buffer);

VkResult
vut_begin_command_buffer(VkCommandBuffer command_buffer);

VkResult
vut_begin_render_pass(VkCommandBuffer buffer,
                      VkRenderPass render_pass,
                      VkFramebuffer framebuffer,
                      VkExtent2D extent);

#endif // HELPER_H