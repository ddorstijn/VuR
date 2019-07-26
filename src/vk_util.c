#include "vk_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENABLE_DEBUG 1

VuResult
vutil_init_window(const char* app_name, GLFWwindow** window)
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
vutil_init_instance(const char* app_name, VkInstance* instance)
{
    // Create app info for Vulkan
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.pEngineName = "No Engine";

    // Get required extensions from GLFW
    uint32_t extension_count;
    const char* const* extensions;
    extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    uint32_t layer_count = 0;
    const char* const* layer_names;

    if (ENABLE_DEBUG) {
        // Create a temporary array for safety with calloc
        char** tmp_extensions;

        tmp_extensions = malloc((extension_count + 1) * sizeof(char*));
        if (tmp_extensions == NULL) {
            return VUR_ALLOC_FAILED;
        }

        // Fill the temporary array with the old data
        for (uint8_t i = 0; i < extension_count; ++i) {
            tmp_extensions[i] = (char*)extensions[i];
        }

        // Add the debug extension
        tmp_extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        // Set the old pointer to the newly created array
        extensions = (const char* const*)tmp_extensions;

        // Add debug layer to enabled layers
        const char* debug_layer[] = { "VK_LAYER_LUNARG_standard_validation" };
        layer_names = (const char* const*)debug_layer;
        layer_count = 1;
    }

    // Create vutil instance
    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = layer_count;
    instance_info.enabledExtensionCount = extension_count;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.ppEnabledLayerNames = layer_names;

    VkResult result = vkCreateInstance(&instance_info, NULL, instance);
    if (result) {
        return VUR_INSTANCE_CREATION_FAILED;
    }

    return VUR_SUCCESS;
}

VuResult
vutil_get_graphics_queue_family_index(VkPhysicalDevice gpu, uint32_t* graphics_queue_family_index)
{
    uint32_t queue_family_count;

    // Retrieve count by passing NULL
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_properties[queue_family_count];

    // Fill the queue family properties array
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_properties);

    // Get the first family which supports graphics
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            *graphics_queue_family_index = i;
        }
    }

    return VUR_SUCCESS;
}

VuResult
vutil_create_device(VkPhysicalDevice gpu, uint32_t graphics_queue_family_index, VkDevice* device)
{
    // When using a single queue no priority is required
    float queue_priority[1] = { 1.0 };

    VkDeviceQueueCreateInfo queue_info;
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.flags = 0;
    queue_info.queueFamilyIndex = graphics_queue_family_index;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priority;

    // Device needs swapchain for displaying graphics
    const char* device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo device_info;
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.flags = 0;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = device_extensions;
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = NULL;
    device_info.pEnabledFeatures = NULL;

    VkResult result = vkCreateDevice(gpu, &device_info, NULL, device);
    if (result) {
        fprintf(stderr, "An unknown error occured while creating a device\n");
        abort();
    }

    return VUR_SUCCESS;
}

VuResult
vutil_get_physical_devices(VkInstance instance, uint32_t* physical_device_count,
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
    *physical_devices = malloc(*physical_device_count * sizeof **physical_devices);
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
vutil_select_suitable_physical_device(VkPhysicalDevice* physical_devices, uint32_t physical_device_count,
                                      uint32_t* physical_device_index)
{
    int discrete_device_index = -1;
    int intergrated_device_index = -1;

    for (int i = 0; i < physical_device_count; i++) {
        uint32_t queue_family_count = 0;
        // Passing in NULL reference sets queue_family_count to the amount of
        // families
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);

        // Allocate the memory for the amount of queue families
        VkQueueFamilyProperties queue_properties[queue_family_count];

        // Fill the queue family properties array
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_properties);

        // Get the first family which supports graphics
        for (uint32_t j = 0; j < queue_family_count; j++) {
            if (queue_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // Check if the device is discrete or intergrated (we want to
                // default to discrete)
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

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
            *physical_device_index = discrete_device_index;
        } else if (intergrated_device_index != -1) {
            *physical_device_index = intergrated_device_index;
        } else {
            return VUR_PHYSICIAL_DEVICE_CREATION_FAILED;
        }
    }

    return VUR_SUCCESS;
}

uint32_t
memory_type_from_properties(VkPhysicalDeviceMemoryProperties memory_properties, uint32_t typeBits,
                            VkFlags requirements_mask, uint32_t* typeIndex)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return 1;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return 0;
}

VuResult
vutil_init_swapchain(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window,
                     VkSwapchainKHR* swapchain)
{
    // max allocation size for multi-buffer swapchain
    // Size of the surface
    // Rotation of the device
    // Behavior of the surface when the output has an alpha channel
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_capabilities);
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

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, present_modes);
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
    if ((surface_capabilities.maxImageCount > 0) && (image_count > surface_capabilities.maxImageCount)) {
        image_count = surface_capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // Prefer non-rotated transformation
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surface_capabilities.currentTransform;
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

    for (uint32_t i = 0; i < sizeof(composite_alpha_flags); i++) {
        if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
            composite_alpha = composite_alpha_flags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_create_info;
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = image_count;
    // swapchain_create_info.imageFormat = format;
    swapchain_create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = NULL;
    swapchain_create_info.preTransform = preTransform;
    swapchain_create_info.compositeAlpha = composite_alpha;
    swapchain_create_info.presentMode = swapchain_present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, swapchain);
    if (result) {
        fprintf(stderr, "Failed to create swapchain\n");
        abort();
    }

    return VUR_SUCCESS;
}

// VuResult
// vutil_init_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, uint32_t* image_count)
// {
//     VkResult result = vkGetSwapchainImagesKHR(device, swapchain, image_count, NULL);
//     if (result) {
//         fprintf(stderr, "Failed to get swapchain images\n");
//         abort();
//     }

//     VkImage swapchain_images[*image_count];
//     result = vkGetSwapchainImagesKHR(device, swapchain, image_count, swapchain_images);
//     if (result) {
//         fprintf(stderr, "Failed to get swapchain images\n");
//         abort();
//     }

//     swapchain_buffers = malloc(*image_count * sizeof(*swapchain_buffers));
//     for (uint32_t i = 0; i < *image_count; ++i) {
//         swapchain_buffers[i].image = swapchain_images[i];

//         VkImageViewCreateInfo image_view_create_info;
//         image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//         image_view_create_info.pNext = NULL;
//         image_view_create_info.flags = 0;
//         image_view_create_info.image = swapchain_buffers[i].image;
//         image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
//         image_view_create_info.format = format;
//         image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
//         image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
//         image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
//         image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
//         image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//         image_view_create_info.subresourceRange.baseMipLevel = 0;
//         image_view_create_info.subresourceRange.levelCount = 1;
//         image_view_create_info.subresourceRange.baseArrayLayer = 0;
//         image_view_create_info.subresourceRange.layerCount = 1;

//         result = vkCreateImageView(device, &image_view_create_info, NULL, &swapchain_buffers[i].view);
//         if (result) {
//             printf("Faild to create image view in loop: %d\n", i);
//         }
//     }
// }

VuResult
vutil_init_render_pass(VkDevice device, VkSwapchainKHR swapchain, VkRenderPass* renderpass)
{
    VkRenderPass* render_pass;

    // Render pass
    VkSemaphore image_acquired_semaphore;
    VkSemaphoreCreateInfo image_acquired_semaphore_create_info;
    image_acquired_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    image_acquired_semaphore_create_info.pNext = NULL;
    image_acquired_semaphore_create_info.flags = 0;

    VkResult result;
    result =
        vkCreateSemaphore(device, &image_acquired_semaphore_create_info, NULL, &image_acquired_semaphore);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create semaphore: %d\n", result);
        abort();
    }

    uint32_t current_buffer;
    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE,
                                   &current_buffer);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to acquire next image: %d\n", result);
        abort();
    }

    // The initial layout for the color and depth attachments will be
    // LAYOUT_UNDEFINED because at the start of the renderpass, we don't
    // care about their contents. At the start of the subpass, the color
    // attachment's layout will be transitioned to
    // LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
    // will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the
    // end of the renderpass, the color attachment's layout will be transitioned
    // to LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as
    // part of the renderpass, no barriers are necessary.
    VkAttachmentDescription attachments[2];
    // attachments[0].format = format;
    // attachments[0].samples = NUM_SAMPLES;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].flags = 0;

    // attachments[1].format = depthStencilContext.format;
    // attachments[1].samples = NUM_SAMPLES;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].flags = 0;

    VkAttachmentReference color_reference;
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference;
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = &depth_reference;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo render_pass_create_info;
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = NULL;
    render_pass_create_info.attachmentCount = 2;
    render_pass_create_info.pAttachments = attachments;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies = NULL;

    result = vkCreateRenderPass(device, &render_pass_create_info, NULL, render_pass);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}