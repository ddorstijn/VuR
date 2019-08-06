#include "renderer.h"

#include "vk_util.h"
#include <stdlib.h>

void
vur_init_vulkan(VulkanContext* ctx, const char* app_name)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    ctx->name = app_name;
    ctx->pause = false;

    vut_init_window(ctx->name, &ctx->window);
    vut_init_instance(ctx->name, &ctx->instance);

    glfwGetFramebufferSize(ctx->window, &ctx->width, &ctx->height);

    // Select the most suitable gpu
    uint32_t gpu_count;
    VkPhysicalDevice* gpus;
    vut_get_physical_devices(ctx->instance, &gpu_count, &gpus);
    vut_pick_physical_device(gpus, gpu_count, &ctx->gpu);

    vut_create_semaphore(ctx->device, &ctx->present_complete);
    vut_create_semaphore(ctx->device, &ctx->render_complete);
    ctx->submit_info = vut_create_submit_info(&ctx->present_complete, &ctx->render_complete);

    // Get a queue which supports graphics and create a logical device
    vut_get_graphics_queue_family_index(ctx->gpus[ctx->gpu_index], &ctx->queueFamilyIndices.graphics);
    vut_create_device(ctx->gpus[ctx->gpu_index], ctx->queueFamilyIndices.graphics, &ctx->device);

    // Let GLFW handle cross-platform surface creation
    glfwCreateWindowSurface(ctx->instance, ctx->window, NULL, &ctx->surface);
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