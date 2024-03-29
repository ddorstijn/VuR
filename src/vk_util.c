/**
 * @file vk_util.c
 * @author Danny Dorstijn (dannydorstijn1997@gmail.com)
 * @brief Vulkan Utility Functions. Initializers and tools
 * @version 0.8
 * @date 2019-08-27
 *
 * @copyright Copyright (c) 2019
 *
 */

#include "vk_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define DEBUG 1

// Helper function (WARNING: DO NOT USE WITH FUNCTION POINTERS, HELL WILL BEFALL ALL)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void
get_required_extensions(uint32_t* extension_count, const char* extensions[])
{
#ifdef DEBUG
    const char* debug[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
#endif // DEBUG

    uint32_t glfw_count = 0;
    const char* const* glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_count);
    if (extensions == NULL) {
        *extension_count = glfw_count;

#ifdef DEBUG
        *extension_count = glfw_count + ARRAY_SIZE(debug);
#endif // DEBUG

        return;
    }

    memcpy(extensions, glfw_extensions, glfw_count * sizeof *extensions);

#ifdef DEBUG
    memcpy(extensions + glfw_count, debug, *extension_count * sizeof *debug);
#endif // DEBUG
}

void error_callback(GLint error, const char* err) {
    printf("%s\n", err);
}

bool
vut_init_window(const char app_name[], GLFWwindow** window)
{
    if (!glfwInit()) {
        // Error
    }

    if (!glfwVulkanSupported()) {
        // Error
    }

    glfwSetErrorCallback(error_callback);

    // Options not necessary for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    *window = glfwCreateWindow(640, 480, app_name, NULL, NULL);
    if (!*window) {
        // Window or OpenGL context creation failed
        printf("Failed to create winodw");
    }

    return true;
}

VkResult
vut_init_instance(const char app_name[], VkInstance* instance)
{
    // Create app info for Vulkan
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = app_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .pEngineName = "No Engine",
    };

    // Get required extensions from GLFW
    uint32_t extension_count = 0;
    get_required_extensions(&extension_count, NULL);

    const char** extensions = malloc(extension_count * sizeof extensions);
    get_required_extensions(&extension_count, extensions);
    
    // Create instance
    const char* layer_names[] = { "VK_LAYER_KHRONOS_validation" };
    const VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
#ifdef DEBUG
        .ppEnabledLayerNames =  layer_names,
        .enabledLayerCount = 1,
#else
        .ppEnabledLayerNames = NULL,
        .enabledLayerCount = 0,
#endif
    };

    VkResult result = vkCreateInstance(&instance_info, NULL, instance);

    free(extensions);
    return result;
}

VkResult
vut_get_physical_devices(VkInstance instance, uint32_t* gpu_count, VkPhysicalDevice gpus[])
{
    // If gpus is NULL then we only get the count. Else we fill the gpu
    VkResult result = vkEnumeratePhysicalDevices(instance, gpu_count, gpus);
    if (result) {
        // Error
        return result;
    }

    if (*gpu_count == 0) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_pick_physical_device(VkPhysicalDevice* gpus, uint32_t gpu_count, VkPhysicalDevice gpu[])
{
    int discrete_device_index = -1;
    int intergrated_device_index = -1;

    for (int i = 0; i < gpu_count; i++) {
        uint32_t queue_family_count = 0;
        // Passing in NULL reference sets queue_family_count to the amount of
        // families
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queue_family_count, NULL);

        // Allocate the memory for the amount of queue families
        VkQueueFamilyProperties queue_properties[queue_family_count];

        // Fill the queue family properties array
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queue_family_count, queue_properties);

        // Get the first family which supports graphics
        for (uint32_t j = 0; j < queue_family_count; j++) {
            if (queue_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // Check if the device is discrete or intergrated (we want to
                // default to discrete)
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(gpus[i], &properties);

                // Determine the type of the physical device
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    // Set the discrete gpu index to this gpu. Ideal
                    discrete_device_index = i;
                    // We can break after this. We have an ideal gpu
                    break;
                } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                    // Set the intergrated gpu index to this device. Less ideal
                    intergrated_device_index = i;
                }
            }
        }

        if (discrete_device_index != -1) {
            *gpu = gpus[discrete_device_index];
        } else if (intergrated_device_index != -1) {
            *gpu = gpus[intergrated_device_index];
        } else {
            // Error
        }
    }

    return VK_SUCCESS;
}

VkResult
vut_get_queue_family_indices(VkPhysicalDevice gpu,
                             VkSurfaceKHR surface,
                             uint32_t* graphics_queue_family_index,
                             uint32_t* present_queue_family_index,
                             bool* separate_present_queue)
{
    // Retrieve count by passing NULL
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_properties[queue_family_count];

    // Fill the queue family properties array
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_properties);

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 supports_present[queue_family_count];
    for (uint32_t i = 0; i < queue_family_count; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &supports_present[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphics_index = UINT32_MAX;
    uint32_t present_index = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if ((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphics_index == UINT32_MAX) {
                graphics_index = i;
            }

            if (supports_present[i] == VK_TRUE) {
                graphics_index = i;
                present_index = i;
                break;
            }
        }
    }

    if (present_index == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < queue_family_count; ++i) {
            if (supports_present[i] == VK_TRUE) {
                present_index = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphics_index == UINT32_MAX || present_index == UINT32_MAX) {
        // Error
    }

    *graphics_queue_family_index = graphics_index;
    *present_queue_family_index = present_index;
    *separate_present_queue = (graphics_index != present_index);

    return true;
}

VkResult
vut_init_device(VkPhysicalDevice gpu, uint32_t graphics_queue_family_index, VkDevice* device)
{
    // When using a single queue no priority is required
    float queue_priority[1] = { 1.0 };

    const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = graphics_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priority,
    };

    // Device needs swapchain for displaying graphics
    const char* device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = device_extensions,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = NULL,
    };

    VkResult result = vkCreateDevice(gpu, &device_info, NULL, device);
    if (result) {
        fprintf(stderr, "An unknown error occured while creating a device\n");
        abort();
    }

    return VK_SUCCESS;
}

VkResult
vut_init_surface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR* surface)
{
    // Let GLFW handle cross-platform surface creation
    if (glfwCreateWindowSurface(instance, window, NULL, surface) != VK_SUCCESS) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_init_semaphore(VkDevice device, VkSemaphore* semaphore)
{
    const VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    VkResult result = vkCreateSemaphore(device, &semaphore_info, NULL, semaphore);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_init_fence(VkDevice device, VkFence* fence)
{
    const VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    VkResult result = vkCreateFence(device, &fence_info, NULL, fence);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

/// Swapchain

VkPresentModeKHR
vut_get_present_mode(VkSurfaceCapabilitiesKHR capabilities,
                     VkPhysicalDevice gpu,
                     VkSurfaceKHR surface)
{
    uint32_t count;
    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, NULL);
    if (result) {
        // Error
    }

    VkPresentModeKHR present_modes[count];
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, present_modes);
    if (result) {
        // Error
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    // Try to find MAILBOX which has the lowest latency without tearing
    // Else look for IMMEDIATE which also has a low latency but can show tearing
    for (uint32_t i = 0; i < count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((present_mode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    return present_mode;
}

VkExtent2D
vut_get_swapchain_extent(VkSurfaceCapabilitiesKHR capabilities, VkExtent2D window_extent)
{
    VkExtent2D extent;
    // Width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF
    if (capabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested
        extent.width = window_extent.width;
        extent.height = window_extent.height;

        if (extent.width < capabilities.minImageExtent.width) {
            extent.width = capabilities.minImageExtent.width;
        } else if (extent.width > capabilities.maxImageExtent.width) {
            extent.width = capabilities.maxImageExtent.width;
        }
        if (extent.height < capabilities.minImageExtent.height) {
            extent.height = capabilities.minImageExtent.height;
        } else if (extent.height > capabilities.maxImageExtent.height) {
            extent.height = capabilities.maxImageExtent.height;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        extent = capabilities.currentExtent;
    }

    return extent;
}

VkResult
vut_get_surface_format(VkPhysicalDevice gpu,
                       VkSurfaceKHR surface,
                       VkFormat* format,
                       VkColorSpaceKHR* color_space)
{
    // Get the list of VkFormat's that are supported:
    uint32_t format_count;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, NULL);
    if (result) {
        // Error
    }

    VkSurfaceFormatKHR surface_formats[format_count];
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, surface_formats);
    if (result) {
        // Error
    }

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        *format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        *format = surface_formats[0].format;
    }
    *color_space = surface_formats[0].colorSpace;

    return VK_SUCCESS;
}

VkResult
vut_init_swapchain(VkPhysicalDevice gpu,
                   VkDevice device,
                   VkSurfaceKHR surface,
                   VkSurfaceCapabilitiesKHR capabilities,
                   VkExtent2D extent,
                   VkFormat format,
                   VkPresentModeKHR present_mode,
                   VkColorSpaceKHR color_space,
                   VkSwapchainKHR* swapchain)
{
    VkSwapchainKHR old_swapchain = *swapchain;

    // Determine the number of VkImage's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    uint32_t image_count = capabilities.minImageCount + 1;
    if ((capabilities.maxImageCount > 0) && (image_count > capabilities.maxImageCount)) {
        image_count = capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // Prefer non-rotated transformation
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        pre_transform = capabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be
    // set
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (uint32_t i = 0; i < 4; i++) {
        if (capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
            composite_alpha = composite_alpha_flags[i];
            break;
        }
    }

    const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = format,
        .imageColorSpace = color_space,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = pre_transform,
        .compositeAlpha = composite_alpha,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };

    VkResult result = vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, swapchain);
    if (result) {
        fprintf(stderr, "Failed to create swapchain\n");
        abort();
    }

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, old_swapchain, NULL);
    }

    return VK_SUCCESS;
}

VkResult
vut_init_image_view(VkDevice device,
                    VkFormat format,
                    VkImage swapchain_image,
                    VkImageView* image_view)
{
    const VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    const VkComponentMapping components = {
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A,
    };

    const VkImageViewCreateInfo color_image_view = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .format = format,
        .components = components,
        .subresourceRange = range,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .image = swapchain_image,
    };

    VkResult result = vkCreateImageView(device, &color_image_view, NULL, image_view);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

// Pipeline
VkResult
read_shader_file(const char* name, size_t* size, uint32_t code[])
{
    if (code == NULL) {
        struct stat sb;
        if (stat(name, &sb) != -1) {
            *size = sb.st_size;
        }

        return VK_SUCCESS;
    }

    FILE* shader;
    shader = fopen(name, "rb");
    if (shader == NULL) {
        printf("  err %d \n", errno);
    }

    size_t bytes_written = fread(code, sizeof(char), *size, shader);
    if (bytes_written != *size) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_init_shader_module(VkDevice device, char* shader_name, VkShaderModule* shader_module)
{
    size_t size;
    read_shader_file(shader_name, &size, NULL);
    uint32_t bytes[size];
    read_shader_file(shader_name, &size, bytes);

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = bytes,
    };

    if (vkCreateShaderModule(device, &createInfo, NULL, shader_module) != VK_SUCCESS) {
        return false;
    }

    return true;
}

VkResult
vut_init_pipeline_layout(VkDevice device,
                         VkDescriptorSetLayout* descriptor_layout,
                         VkPipelineLayout* pipeline_layout)
{
    const VkPipelineLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .setLayoutCount = 0,
        .pSetLayouts = descriptor_layout,
    };

    VkResult result = vkCreatePipelineLayout(device, &create_info, NULL, pipeline_layout);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_prepare_render_pass(VkDevice device, VkFormat surface_format, VkRenderPass* render_pass)
{
    VkAttachmentDescription color_attachment = {
        .format = surface_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkResult result = vkCreateRenderPass(device, &render_pass_info, NULL, render_pass);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

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
                  VkPipeline* pipeline)
{
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = vertex_input,
        .pInputAssemblyState = input_assembly,
        .pViewportState = viewport_state,
        .pRasterizationState = rasterizer,
        .pMultisampleState = multisampling,
        .pDepthStencilState = NULL,
        .pColorBlendState = color_blending,
        .pDynamicState = NULL,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline) !=
        VK_SUCCESS) {
        return false;
    }

    return true;
}

VkResult
vut_prepare_framebuffer(VkDevice device,
                        VkRenderPass render_pass,
                        VkImageView image_view,
                        VkExtent2D window_extent,
                        VkFramebuffer* framebuffer)
{
    VkImageView attachments[] = { image_view };

    const VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = window_extent.width,
        .height = window_extent.height,
        .layers = 1,
    };

    VkResult result = vkCreateFramebuffer(device, &create_info, NULL, framebuffer);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

// Command Buffers

VkResult
vut_init_command_pool(VkDevice device, uint32_t family_index, VkCommandPool* command_pool)
{
    const VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = family_index,
    };

    VkResult result = vkCreateCommandPool(device, &command_pool_info, NULL, command_pool);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_alloc_command_buffer(VkDevice device,
                         VkCommandPool command_pool,
                         uint32_t count,
                         VkCommandBuffer* command_buffer)
{
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count,
    };

    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, command_buffer);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_begin_command_buffer(VkCommandBuffer command_buffer)
{
    const VkCommandBufferBeginInfo buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    VkResult result = vkBeginCommandBuffer(command_buffer, &buffer_begin_info);
    if (result) {
        // Error
    }

    return VK_SUCCESS;
}

VkResult
vut_begin_render_pass(VkCommandBuffer buffer,
                      VkRenderPass render_pass,
                      VkFramebuffer framebuffer,
                      VkExtent2D extent)
{
    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

    const VkRenderPassBeginInfo render_pass_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea.offset = { 0, 0 },
        .renderArea.extent = extent,
        .clearValueCount = 1,
        .pClearValues = &clearColor,
    };

    vkCmdBeginRenderPass(buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

    return VK_SUCCESS;
}