//#define NDEBUG
#include <assert.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "renderer.h"


#define WIDTH 800
#define HEIGHT 600
#define MAX_NUM_IMAGES 4


#define ERR_EXIT(err_msg, err_class)                                           \
    do {                                                                       \
        printf(err_msg);                                                       \
        fflush(stdout);                                                        \
        exit(1);                                                               \
    } while (0)


struct buffer {
   VkDeviceMemory mem;
   VkImage image;
   VkImageView view;
   VkFramebuffer framebuffer;
   VkFence fence;
   VkCommandBuffer cmd_buffer;
};

static struct {
	GLFWwindow* window;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice phy_device;
	VkDevice log_device;

	struct {
		VkQueue handle;
		uint32_t idx;
	} queue;

	VkRenderPass renderpass;
	VkCommandPool cmd_pool;
	VkSemaphore semaphore;

	VkSwapchainKHR swapchain;
	VkExtent2D window_size;
	VkColorSpaceKHR colorspace;
	VkFormat format;

	uint32_t n_swapchain_images;
	struct buffer buffers[MAX_NUM_IMAGES];
} ctx;

static void
vur_init_window()
{
    if (!glfwInit()) {
        printf("Cannot initialize GLFW.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    if (!glfwVulkanSupported()) {
        printf("GLFW failed to find the Vulkan loader.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	ctx.window = glfwCreateWindow(WIDTH, HEIGHT, "", NULL, NULL);
	assert(ctx.window != NULL);
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

#ifdef NDEBUG
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

	VkResult res = vkCreateInstance(&instance_info, NULL, &ctx.instance);
	if (res) {
		if (res == VK_ERROR_INCOMPATIBLE_DRIVER) {
			ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
					 "(ICD).\n\nPlease look at the Getting Started guide for "
					 "additional information.\n",
					 "vkCreateInstance Failure");
		} else if (res == VK_ERROR_EXTENSION_NOT_PRESENT) {
			ERR_EXIT("Cannot find a specified extension library"
					 ".\nMake sure your layers path is set appropriately\n",
					 "vkCreateInstance Failure");
		} else if (res) {
			ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
					 "installable client driver (ICD) installed?\nPlease look at "
					 "the Getting Started guide for additional information.\n",
					 "vkCreateInstance Failure");
		}
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
		ERR_EXIT("No suitable GPU has been found.\nExiting...\n", "Pick physical device");
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
        .queueFamilyIndex = ctx.queue.idx,
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

	vkGetDeviceQueue(ctx.log_device, ctx.queue.idx, 0, &ctx.queue.handle);
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
    uint32_t idx_queue = UINT32_MAX;
    for (uint32_t i = 0; i < n_queue_families; i++) {
        if ((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && supports_present[i] == VK_TRUE) {
			idx_queue = i;
			break;
		}
    }

    if (idx_queue == UINT32_MAX) {
		ERR_EXIT("Couldn't find a queue that supports both graphics and present\nExiting...\n", "Get queue family indices");
    }

    ctx.queue.idx = idx_queue;
}

static void
vur_create_objects()
{
	VkAttachmentDescription color_attachment = {
        .format = ctx.format,
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

    VkRenderPassCreateInfo renderpass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

	vkCreateRenderPass(ctx.log_device, &renderpass_info, NULL, &ctx.renderpass);

	const VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = 0
    };

	vkCreateCommandPool(ctx.log_device, &command_pool_create_info, NULL, &ctx.cmd_pool);

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	vkCreateSemaphore(ctx.log_device, &semaphore_info, NULL, &ctx.semaphore);
}

static void
vur_get_surface_format()
{
    // Get the list of VkFormat's that are supported:
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.phy_device, ctx.surface, &format_count, NULL);
    VkSurfaceFormatKHR surface_formats[format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.phy_device, ctx.surface, &format_count, surface_formats);

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        ctx.format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        ctx.format = surface_formats[0].format;
    }
	
    ctx.colorspace = surface_formats[0].colorSpace;
}

static void
init_buffer(struct buffer *b)
{
	const VkImageViewCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = b->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = ctx.format,
        .components = {
			.r = VK_COMPONENT_SWIZZLE_R,
			.g = VK_COMPONENT_SWIZZLE_G,
			.b = VK_COMPONENT_SWIZZLE_B,
			.a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
        },
	};
	vkCreateImageView(ctx.log_device, &image_info, NULL, &b->view);

	VkFramebufferCreateInfo framebuffer_info = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = ctx.renderpass,
		.attachmentCount = 1,
		.pAttachments = &b->view,
		.width = ctx.window_size.width,
		.height = ctx.window_size.height,
		.layers = 1
	};
	vkCreateFramebuffer(ctx.log_device, &framebuffer_info, NULL, &b->framebuffer);

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	vkCreateFence(ctx.log_device, &fence_info, NULL, &b->fence);

	VkCommandBufferAllocateInfo command_buffer_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = ctx.cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
   	vkAllocateCommandBuffers(ctx.log_device, &command_buffer_info, &b->cmd_buffer);
}

static void
vur_create_swapchain()
{
	
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.phy_device, ctx.surface, &capabilities);	

	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(ctx.phy_device, 0, ctx.surface, &supported);
	if (!supported) {
		// Error
	}

	uint32_t n_present_modes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.phy_device, ctx.surface, &n_present_modes, NULL);
	VkPresentModeKHR present_modes[n_present_modes];
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.phy_device, ctx.surface, &n_present_modes, present_modes);

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < n_present_modes; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}

	uint32_t n_images = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && n_images > capabilities.maxImageCount) {
    	n_images = capabilities.maxImageCount;
	}

	ctx.window_size = capabilities.currentExtent;
	if (capabilities.currentExtent.width == UINT32_MAX) {
		VkExtent2D extent = {WIDTH, HEIGHT};

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

		ctx.window_size = extent;
	}

	const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = ctx.surface,
        .minImageCount = n_images,
        .imageFormat = ctx.format,
        .imageColorSpace = ctx.colorspace,
        .imageExtent = ctx.window_size,
        .imageArrayLayers = 1,

 		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = NULL,
    };

    if ( vkCreateSwapchainKHR(ctx.log_device, &swapchain_create_info, NULL, &ctx.swapchain) != VK_SUCCESS) {
		// Error
    }

	vkGetSwapchainImagesKHR(ctx.log_device, ctx.swapchain, &ctx.n_swapchain_images, NULL);
	VkImage swap_chain_images[ctx.n_swapchain_images];
	vkGetSwapchainImagesKHR(ctx.log_device, ctx.swapchain, &ctx.n_swapchain_images, swap_chain_images);

	for (uint32_t i = 0; i < ctx.n_swapchain_images; i++) {
		ctx.buffers[i].image = swap_chain_images[i];
		init_buffer(&ctx.buffers[i]);
	}
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
	vur_get_surface_format();

	vur_create_objects();
	vur_create_swapchain();
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
	for (uint32_t i = 0; i < ctx.n_swapchain_images; i++) {
		vkDestroyImageView(ctx.log_device, ctx.buffers[i].view, NULL);
		vkDestroyFramebuffer(ctx.log_device, ctx.buffers[i].framebuffer, NULL);
		vkDestroyFence(ctx.log_device, ctx.buffers[i].fence, NULL);
	}
	vkDestroyCommandPool(ctx.log_device, ctx.cmd_pool, NULL);
	vkDestroySemaphore(ctx.log_device, ctx.semaphore, NULL);
	vkDestroySwapchainKHR(ctx.log_device, ctx.swapchain, NULL);
	vkDestroyRenderPass(ctx.log_device, ctx.renderpass, NULL);
	vkDestroyDevice(ctx.log_device, NULL);
	vkDestroySurfaceKHR(ctx.instance, ctx.surface, NULL);
	vkDestroyInstance(ctx.instance, NULL);
	glfwDestroyWindow(ctx.window);
	glfwTerminate();
}
