#include "renderer.h"

#include "vk_util.h"
#include <stdlib.h>

VulkanContext*
vur_init_vulkan(const char* app_name)
{
    VulkanContext* ctx;
    ctx = malloc(sizeof *ctx);
    if (ctx == NULL) {
        // Error
    }

    ctx->name = app_name;
    vutil_init_window(ctx->name, &ctx->window);
    vutil_init_instance(ctx->name, &ctx->instance);

    // Select best gpu
    vutil_get_physical_devices(ctx->instance, &ctx->gpu_count, &ctx->gpus);
    vutil_select_suitable_physical_device(ctx->gpus, ctx->gpu_count, &ctx->gpu_index);

    // Set the memory properties for this device
    vkGetPhysicalDeviceMemoryProperties(ctx->gpus[ctx->gpu_index], &ctx->memory_properties);
    vutil_get_graphics_queue_family_index(ctx->gpus[ctx->gpu_index], &ctx->graphics_queue_family_index);

    vutil_create_device(ctx->gpus[ctx->gpu_index], ctx->graphics_queue_family_index, &ctx->device);

    glfwCreateWindowSurface(ctx->instance, ctx->window, NULL, &ctx->surface);

    return ctx;
}

// Main render loop
void
vur_draw(VulkanContext* ctx)
{
    while (!glfwWindowShouldClose(ctx->window)) {
        glfwPollEvents();
    }
}

void
vur_destroy(VulkanContext* ctx)
{
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    vkDestroyDevice(ctx->device, NULL);
    vkDestroyInstance(ctx->instance, NULL);
    glfwTerminate();
    free(ctx);
}