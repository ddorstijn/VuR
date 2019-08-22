#include "vk_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENABLE_DEBUG 1

// Helper function (WARNING: DO NOT USE WITH FUNCTION POINTERS, HELL WILL BEFALL ALL)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

VuResult
vut_init_window(const char* app_name, GLFWwindow** window)
{
    if (!glfwInit()) {
        return VUR_GLFW_INIT_FAILED;
    }

    if (!glfwVulkanSupported()) {
        return VUR_VULKAN_NOT_SUPPORTED;
    }

    // Options not necessary for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    *window = glfwCreateWindow(640, 480, app_name, NULL, NULL);
    if (!*window) {
        // Window or OpenGL context creation failed
        return VUR_WINDOW_CREATION_FAILED;
    }

    return VUR_SUCCESS;
}

VuResult
vut_init_instance(const char* app_name, VkInstance* instance)
{
    // Create app info for Vulkan
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .pEngineName = "No Engine",
    };

    // Get required extensions from GLFW
    uint32_t extension_count;
    const char* const* extensions = glfwGetRequiredInstanceExtensions(&extension_count);

#ifdef ENABLE_DEBUG
    // Add debug layer to enabled layers
    const char* debug_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
#endif

    // Create vut instance
    const VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
#ifdef ENABLE_DEBUG
        .ppEnabledLayerNames = debug_layers,
        .enabledLayerCount = ARRAY_SIZE(debug_layers),
#endif
    };

    VkResult result = vkCreateInstance(&instance_info, NULL, instance);
    if (result) {
        return VUR_INSTANCE_CREATION_FAILED;
    }

    return VUR_SUCCESS;
}

VuResult
vut_get_queue_family_indices(VkPhysicalDevice gpu,
                             VkSurfaceKHR surface,
                             uint32_t* queue_family_count,
                             uint32_t* graphics_queue_family_index,
                             uint32_t* present_queue_family_index,
                             bool* separate_present_queue)
{
    // Retrieve count by passing NULL
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, queue_family_count, NULL);
    VkQueueFamilyProperties queue_properties[*queue_family_count];

    // Fill the queue family properties array
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, queue_family_count, queue_properties);

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 supportsPresent[*queue_family_count];
    for (uint32_t i = 0; i < *queue_family_count; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < *queue_family_count; i++) {
        if ((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueFamilyIndex == UINT32_MAX) {
                graphicsQueueFamilyIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueFamilyIndex = i;
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    if (presentQueueFamilyIndex == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < *queue_family_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
        // Error
    }

    *graphics_queue_family_index = graphicsQueueFamilyIndex;
    *present_queue_family_index = presentQueueFamilyIndex;
    *separate_present_queue = (graphics_queue_family_index != present_queue_family_index);
}

VuResult
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

    return VUR_SUCCESS;
}

VuResult
vut_get_physical_devices(VkInstance instance,
                         uint32_t* physical_device_count,
                         VkPhysicalDevice** physical_devices)
{
    // Get the number of devices (GPUs) available.
    VkResult result = vkEnumeratePhysicalDevices(instance, physical_device_count, NULL);
    if (result) {
        VUR_DEVICE_CREATION_FAILED;
    }

    if (*physical_device_count == 0) {
        VUR_DEVICE_CREATION_FAILED;
    }

    // Allocate space and get the list of devices.
    *physical_devices =
      (VkPhysicalDevice*)malloc(*physical_device_count * sizeof **physical_devices);
    if (*physical_devices == NULL) {
        fprintf(stderr, "Couldn't allocate memory for physical device list\n");
        abort();
    }

    // Fill the array with the devices
    result = vkEnumeratePhysicalDevices(instance, physical_device_count, *physical_devices);
    if (result) {
        return VUR_DEVICE_CREATION_FAILED;
    }

    return VUR_SUCCESS;
}

VuResult
vut_pick_physical_device(VkPhysicalDevice* gpus, uint32_t gpu_count, VkPhysicalDevice* gpu)
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
            return VUR_PHYSICIAL_DEVICE_CREATION_FAILED;
        }
    }

    return VUR_SUCCESS;
}

VuResult
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

    return VUR_SUCCESS;
}

VuResult
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

    return VUR_SUCCESS;
}

VuResult
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

    return VUR_SUCCESS;
}

VuResult
vut_alloc_command_buffer(VkDevice device,
                         VkCommandPool command_pool,
                         VkCommandBuffer* command_buffer)
{
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, command_buffer);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

VuResult
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

    return VUR_SUCCESS;
}

VuResult
vut_get_format(VkPhysicalDevice gpu,
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

    return VUR_SUCCESS;
}

VuResult
vut_init_swapchain(VkPhysicalDevice gpu,
                   VkDevice device,
                   VkSurfaceKHR surface,
                   GLFWwindow* window,
                   VkSwapchainKHR* swapchain,
                   VkFormat* format,
                   VkColorSpaceKHR* color_space)
{
    VkSwapchainKHR old_swapchain = *swapchain;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult result =
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_capabilities);
    if (result) {
        fprintf(stderr, "error getting surface capabilties\n");
        abort();
    }

    uint32_t present_mode_count;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, NULL);
    if (result) {
        fprintf(stderr, "Error getting present modes\n");
        abort();
    }

    VkPresentModeKHR present_modes[present_mode_count];

    result =
      vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, present_modes);
    if (result) {
        fprintf(stderr, "Error filling present modes\n");
        abort();
    }

    VkExtent2D swapchain_extent;
    // Width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF
    if (surface_capabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        swapchain_extent.width = (uint32_t)width;
        swapchain_extent.height = (uint32_t)height;

        if (swapchain_extent.width < surface_capabilities.minImageExtent.width) {
            swapchain_extent.width = surface_capabilities.minImageExtent.width;
        } else if (swapchain_extent.width > surface_capabilities.maxImageExtent.width) {
            swapchain_extent.width = surface_capabilities.maxImageExtent.width;
        }
        if (swapchain_extent.height < surface_capabilities.minImageExtent.height) {
            swapchain_extent.height = surface_capabilities.minImageExtent.height;
        } else if (swapchain_extent.height > surface_capabilities.maxImageExtent.height) {
            swapchain_extent.height = surface_capabilities.maxImageExtent.height;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchain_extent = surface_capabilities.currentExtent;
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    // If v-sync is not requested, try to find a mailbox mode
    // It's the lowest latency non-tearing present mode available
    // if (!vsync) {
    for (size_t i = 0; i < present_mode_count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // Determine the number of VkImage's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if ((surface_capabilities.maxImageCount > 0) &&
        (image_count > surface_capabilities.maxImageCount)) {
        image_count = surface_capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // Prefer non-rotated transformation
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        pre_transform = surface_capabilities.currentTransform;
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
        if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
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
        .imageFormat = *format,
        .imageColorSpace = *color_space,
        .imageExtent = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = pre_transform,
        .compositeAlpha = composite_alpha,
        .presentMode = swapchain_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };

    result = vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, swapchain);
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

    return VUR_SUCCESS;
}

VuResult
vut_init_image(VkDevice device, VkFormat format, uint32_t width, uint32_t height, VkImage* image)
{
    const VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { width, height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0,
    };

    VkResult result = vkCreateImage(device, &image_info, NULL, image);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

VuResult
vut_init_image_view(VkDevice device,
                    VkFormat format,
                    VkImage swapchain_image,
                    VkImageView* image_view,
                    bool is_depth)
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

    return VUR_SUCCESS;
}

VuResult
vut_init_pipeline(VkPipelineLayout pipeline_layout)
{}

VuResult
vut_init_pipeline_layout(VkDevice device,
                         VkDescriptorSetLayout* descriptor_layout,
                         VkPipelineLayout* pipeline_layout)
{
    const VkPipelineLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .setLayoutCount = 1,
        .pSetLayouts = descriptor_layout,
    };

    VkResult result = vkCreatePipelineLayout(device, &create_info, NULL, pipeline_layout);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

VuResult
vut_init_descriptor_layout(VkDevice device, VkDescriptorSetLayout* descriptor_layout)
{
    const VkDescriptorSetLayoutBinding layout_bindings[2] = {
        [0] =
          {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = NULL,
          },
        [1] =
          {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = DEMO_TEXTURE_COUNT,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL,
          },
    };
    const VkDescriptorSetLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .bindingCount = 2,
        .pBindings = layout_bindings,
    };

    VkResult result = vkCreateDescriptorSetLayout(device, &create_info, NULL, descriptor_layout);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

VuResult
vut_init_descriptor_pool(VkDevice device, uint32_t image_count, VkDescriptorPool* descriptor_pool)
{
    const VkDescriptorPoolSize type_counts[2] = {
        [0] =
          {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = image_count,
          },
        [1] =
          {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = image_count * DEMO_TEXTURE_COUNT,
          },
    };
    const VkDescriptorPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = image_count,
        .poolSizeCount = 2,
        .pPoolSizes = type_counts,
    };

    VkResult result = vkCreateDescriptorPool(device, &create_info, NULL, descriptor_pool);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

VuResult
vut_init_descriptor_set(VkDevice device,
                        VkSampler sampler,
                        VkImageView view,
                        VkDescriptorPool pool,
                        VkDescriptorSetLayout* layout,
                        VkDescriptorSet* set)
{
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[2];

    const VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = layout,
    };

    VkDescriptorBufferInfo buffer_info = {
        .offset = 0,
        .range = sizeof(struct vktexcube_vs_uniform),
    };

    tex_descs[i].sampler = sampler;
    tex_descs[i].imageView = view;
    tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &buffer_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;

    for (unsigned int i = 0; i < image_count; i++) {
        VkResult result = vkAllocateDescriptorSets(device, &alloc_info, set);
        if (result) {
            // Error
        }

        buffer_info.buffer = swapchain_image_resources[i].uniform_buffer;
        writes[0].dstSet = swapchain_image_resources[i].descriptor_set;
        writes[1].dstSet = swapchain_image_resources[i].descriptor_set;
        vkUpdateDescriptorSets(device, 2, writes, 0, NULL);
    }
}

VuResult
vut_init_render_pass(VkDevice device,
                     VkFormat color_format,
                     VkFormat depth_format,
                     VkRenderPass* render_pass)
{
    const VkAttachmentDescription attachments[2] = {
        [0] =
          {
            .format = color_format,
            .flags = 0,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          },
        [1] =
          {
            .format = depth_format,
            .flags = 0,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          },
    };
    const VkAttachmentReference color_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .flags = 0,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_reference,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = &depth_reference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };
    const VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };

    VkResult result = vkCreateRenderPass(device, &create_info, NULL, render_pass);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

VuResult
vut_init_framebuffer(VkDevice device,
                     VkRenderPass render_pass,
                     VkImageView depth_view,
                     VkImageView image_view,
                     uint32_t width,
                     uint32_t height,
                     VkFramebuffer* framebuffer)
{
    VkImageView attachments[2];
    attachments[1] = depth_view;

    const VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = render_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = width,
        .height = height,
        .layers = 1,
    };

    attachments[0] = image_view;
    VkResult result = vkCreateFramebuffer(device, &create_info, NULL, framebuffer);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

const VkSubmitInfo
vut_init_submit_info(VkSemaphore* present_complete, VkSemaphore* render_complete)
{
    const VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitDstStageMask = flags,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = present_complete,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = render_complete,
    };

    return submit_info;
}

VuResult
vut_build_image_ownership_cmd(VkCommandBuffer present_buffer,
                              uint32_t graphics_queue_family_index,
                              uint32_t present_queue_family_index,
                              VkImage swapchain_image)
{
    VkResult result;

    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL,
    };
    result = vkBeginCommandBuffer(present_buffer, &cmd_buf_info);
    if (result) {
        // Error
    }

    const VkImageMemoryBarrier image_ownership_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = graphics_queue_family_index,
        .dstQueueFamilyIndex = present_queue_family_index,
        .image = swapchain_image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };

    vkCmdPipelineBarrier(present_buffer,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         NULL,
                         0,
                         NULL,
                         1,
                         &image_ownership_barrier);
    result = vkEndCommandBuffer(present_buffer);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}