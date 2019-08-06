#include "vk_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENABLE_DEBUG 1

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
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.pEngineName = "No Engine";

    // Get required extensions from GLFW
    uint32_t extension_count;
    const char* const* extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    // Create vut instance
    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = extension_count;
    instance_info.ppEnabledExtensionNames = extensions;

#ifdef ENABLE_DEBUG
    // Add debug layer to enabled layers
    const char* debug_layers[] = { "VK_LAYER_LUNARG_standard_validation" };

    instance_info.ppEnabledLayerNames = debug_layers;
    instance_info.enabledLayerCount = sizeof(debug_layers) / sizeof(debug_layers[0]);
#endif

    VkResult result = vkCreateInstance(&instance_info, NULL, instance);
    if (result) {
        return VUR_INSTANCE_CREATION_FAILED;
    }

    return VUR_SUCCESS;
}

VuResult
vut_get_queue_family_index(VkPhysicalDevice gpu, uint32_t* graphics_queue_family_index)
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
vut_create_device(VkPhysicalDevice gpu, uint32_t graphics_queue_family_index, VkDevice* device)
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
vut_get_physical_devices(VkInstance instance, uint32_t* physical_device_count,
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
    *physical_devices = (VkPhysicalDevice*)malloc(*physical_device_count * sizeof **physical_devices);
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
vut_pick_physical_device(VkPhysicalDevice* physical_devices, uint32_t physical_device_count,
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

VuResult
vut_create_semaphore(VkDevice device, VkSemaphore* semaphore)
{
    VkSemaphoreCreateInfo semaphore_info;
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = NULL;
    semaphore_info.flags = 0;

    vkCreateSemaphore(device, &semaphore_info, NULL, semaphore);
}

VuResult
vut_create_command_pool(VkDevice device, uint32_t family_index, VkCommandPool* command_pool)
{
    VkCommandPoolCreateInfo command_pool_info;
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    command_pool_info.flags = 0;
    command_pool_info.queueFamilyIndex = family_index;

    vkCreateCommandPool(device, &command_pool_info, NULL, command_pool);
}

VuResult
vut_init_swapchain(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window,
                   VkSwapchainKHR* swapchain, VkFormat* format, VkColorSpaceKHR* color_space)
{
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

    // Get the list of VkFormat's that are supported:
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, NULL);

    VkSurfaceFormatKHR surface_formats[format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, surface_formats);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        *format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        *format = surface_formats[0].format;
    }
    *color_space = surface_formats[0].colorSpace;

    VkSwapchainCreateInfoKHR swapchain_create_info;
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = *format;
    swapchain_create_info.imageColorSpace = *color_space;
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

VuResult
vut_init_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, VkFormat format, uint32_t* image_count)
{
    return VUR_SUCCESS;
}

VuResult
vut_init_render_pass(VkDevice device, VkFormat format, VkRenderPass* render_pass)
{
    VkAttachmentDescription attachments[1] = {};
    attachments[0].format = format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachments = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachments;

    VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkResult result = vkCreateRenderPass(device, &createInfo, 0, render_pass);
    if (result) {
        // Error
    }

    return VUR_SUCCESS;
}

VkFramebuffer
createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView image_view, GLFWwindow* window,
                  VkFramebuffer* framebuffer)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &image_view;
    createInfo.width = (uint32_t)width;
    createInfo.height = (uint32_t)height;
    createInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(device, &createInfo, 0, framebuffer);
    if (result) {
        // Error
    }
}

VkSubmitInfo
vut_create_submit_info(VkSemaphore* present_complete, VkSemaphore* render_complete)
{
    VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitDstStageMask = flags;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = present_complete;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = render_complete;

    return submit_info;
}