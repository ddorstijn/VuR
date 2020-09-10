#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "renderer.h"

#define DEBUG

struct queue {
	VkQueue handle;
	uint32_t idx;
};

static struct {
	GLFWwindow* window;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDevice log_device;

	VkPhysicalDevice phy_device;

	struct queue graphics_queue;
	struct queue present_queue;
} ctx;

static void
vur_init_window()
{
	glfwInit();

	if (!glfwVulkanSupported()) {
		exit(1);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	ctx.window = glfwCreateWindow(800, 600, "", NULL, NULL);
}

static void
vur_init_instance()
{
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "VuR",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .pEngineName = "VuR Framework",
		.engineVersion = VK_MAKE_VERSION(0, 1, 1)
    };

#ifndef DEBUG
	uint32_t n_extensions = 0;
	char** extensions = (char**)glfwGetRequiredInstanceExtensions(&n_extensions);

	uint32_t n_layers = 0;
	const char** layers = NULL;
#else
	uint32_t n_extensions = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&n_extensions);
	char* extensions[++n_extensions];
	extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	for (uint32_t i = 1; i < n_extensions; i++) {
		extensions[i] = (char*)glfw_extensions[i - 1];
	}

	uint32_t n_layers = 1;
	const char** layers = (const char* []){ "VK_LAYER_KHRONOS_validation" };
	
#endif

	const VkInstanceCreateInfo instance_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.flags = 0,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = n_extensions,
		.ppEnabledExtensionNames = (const char**)extensions,
        .ppEnabledLayerNames = layers,
        .enabledLayerCount = n_layers,
	};
	
	if (vkCreateInstance(&instance_info, NULL, &ctx.instance) != VK_SUCCESS) {
		exit(1);
	}
}

static void
vur_pick_physical_device()
{
	uint32_t n_phy_devices;	
	vkEnumeratePhysicalDevices(ctx.instance, &n_phy_devices, NULL);
	VkPhysicalDevice phy_devices[n_phy_devices];
	vkEnumeratePhysicalDevices(ctx.instance, &n_phy_devices, phy_devices);

    int idx_intergrated_device = -1;

    for (int i = 0; i < n_phy_devices; i++) {
        uint32_t n_queue_families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phy_devices[i], &n_queue_families, NULL);
        VkQueueFamilyProperties queue_properties[n_queue_families];
        vkGetPhysicalDeviceQueueFamilyProperties(phy_devices[i], &n_queue_families, queue_properties);

        for (uint32_t j = 0; j < n_queue_families; j++) {
            if (queue_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(phy_devices[i], &properties);

                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					ctx.phy_device = phy_devices[i];
                    return;
                } 

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                    idx_intergrated_device = i;
                }
            }
        }
    }

	if (idx_intergrated_device != -1) {
		ctx.phy_device = phy_devices[idx_intergrated_device];
	} else {
		// Error
	}
}

static void
vur_create_logical_device()
{
	// When using a single queue no priority is required
    float queue_priority[1] = { 1.0 };

    const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = ctx.graphics_queue.idx,
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

    if (vkCreateDevice(ctx.phy_device, &device_info, NULL, &ctx.log_device) != VK_SUCCESS) {
		// Error
    }	

	vkGetDeviceQueue(ctx.log_device, ctx.graphics_queue.idx, 0, &ctx.graphics_queue.handle);
    vkGetDeviceQueue(ctx.log_device, ctx.present_queue.idx, 0, &ctx.present_queue.handle);
}

static void
vur_get_queue_family_indices()
{
    uint32_t n_queue_families;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.phy_device, &n_queue_families, NULL);
    VkQueueFamilyProperties queue_properties[n_queue_families];
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.phy_device, &n_queue_families, queue_properties);

    VkBool32 supports_present[n_queue_families];
    for (uint32_t i = 0; i < n_queue_families; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx.phy_device, i, ctx.surface, &supports_present[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphics_index = UINT32_MAX;
    uint32_t present_index = UINT32_MAX;
    for (uint32_t i = 0; i < n_queue_families; i++) {
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
        for (uint32_t i = 0; i < n_queue_families; ++i) {
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

    ctx.graphics_queue.idx = graphics_index;
    ctx.present_queue.idx = present_index;
}

void
vur_init()
{
	vur_init_window();
	vur_init_instance();

	if (glfwCreateWindowSurface(ctx.instance, ctx.window, NULL, &ctx.surface) != VK_SUCCESS) {
        // Error
    }

	vur_pick_physical_device();
	vur_get_queue_family_indices();
	vur_create_logical_device();
}

int
vur_draw()
{
	if (glfwWindowShouldClose(ctx.window)) {
		return 0;
	}

	glfwPollEvents();

	return 1;
}

void
vur_destroy()
{
	vkDestroyDevice(ctx.log_device, NULL);
	vkDestroySurfaceKHR(ctx.instance, ctx.surface, NULL);
	vkDestroyInstance(ctx.instance, NULL);
	glfwDestroyWindow(ctx.window);
	glfwTerminate();
}
