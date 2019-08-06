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

    // Let GLFW handle cross-platform surface creation
    glfwCreateWindowSurface(ctx->instance, ctx->window, NULL, &ctx->surface);
    glfwGetFramebufferSize(ctx->window, &ctx->width, &ctx->height);

    // Select the most suitable gpu
    uint32_t gpu_count;
    VkPhysicalDevice* gpus;
    vut_get_physical_devices(ctx->instance, &gpu_count, &gpus);
    vut_pick_physical_device(gpus, gpu_count, &ctx->gpu);

    // Get a queue which supports graphics and create a logical device
    vut_get_queue_family_indices(ctx->gpu, ctx->surface, &ctx->queue_family_count,
                                 &ctx->graphics_queue_family_index, &ctx->present_queue_family_index,
                                 ctx->separate_present_queue);

    vut_create_device(ctx->gpu, ctx->graphics_queue_family_index, &ctx->device);

    // Store the correct queues from indices
    vkGetDeviceQueue(ctx->device, ctx->graphics_queue_family_index, 0, &ctx->graphics_queue);

    if (!ctx->separate_present_queue) {
        ctx->present_queue = ctx->graphics_queue;
    } else {
        vkGetDeviceQueue(ctx->device, ctx->present_queue_family_index, 0, &ctx->present_queue);
    }

    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting

    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents
    for (uint32_t i = 0; i < FRAME_LAG; i++) {
        vut_create_fence(ctx->device, &ctx->fences[i]);
        vut_create_semaphore(ctx->device, &ctx->image_acquired_semaphores[i]);
        vut_create_semaphore(ctx->device, &ctx->draw_complete_semaphores[i]);

        if (ctx->separate_present_queue) {
            vut_create_semaphore(ctx->device, &ctx->image_ownership_semaphores[i]);
        }
    }
}

void
vur_prepare_buffers(VulkanContext* ctx)
{
    if (ctx->command_pool == VK_NULL_HANDLE) {
        vut_create_command_pool(ctx->device, ctx->graphics_queue_family_index, &ctx->command_pool);
    }
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