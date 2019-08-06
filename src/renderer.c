#include "renderer.h"

#include "vk_util.h"
#include <stdlib.h>

VulkanContext*
vur_init_vulkan(const char* app_name)
{
    VulkanContext* ctx;
    ctx = (VulkanContext*)malloc(sizeof *ctx);
    if (ctx == NULL) {
        // Error
    }

    ctx->name = app_name;
    vut_init_window(ctx->name, &ctx->window);
    vut_init_instance(ctx->name, &ctx->instance);

    // Select the most suitable gpu
    vut_get_physical_devices(ctx->instance, &ctx->gpu_count, &ctx->gpus);
    vut_pick_physical_device(ctx->gpus, ctx->gpu_count, &ctx->gpu_index);

    vut_create_semaphore(ctx->device, &ctx->present_complete);
    vut_create_semaphore(ctx->device, &ctx->render_complete);
    ctx->submit_info = vut_create_submit_info(&ctx->present_complete, &ctx->render_complete);

    // Get a queue which supports graphics and create a logical device
    vut_get_graphics_queue_family_index(ctx->gpus[ctx->gpu_index], &ctx->queueFamilyIndices.graphics);
    vut_create_device(ctx->gpus[ctx->gpu_index], ctx->queueFamilyIndices.graphics, &ctx->device);

    // Let GLFW handle cross-platform surface creation
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