#include "renderer.h"

#include "vk_util.h"
#include <stdlib.h>

static void
vur_framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    VulkanContext* ctx = (VulkanContext*)glfwGetWindowUserPointer(window);
    ctx->framebuffer_resized = true;
}

void
vur_init_vulkan(VulkanContext* ctx, const char* app_name)
{
    // Make sure the whole struct is NULL
    memset(ctx, 0, sizeof(*ctx));

    ctx->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    ctx->name = app_name;
    ctx->pause = false;

    vut_init_window(ctx->name, &ctx->window);
    glfwSetWindowUserPointer(ctx->window, ctx);
    glfwSetFramebufferSizeCallback(ctx->window, vur_framebuffer_resize_callback);

    // Initialize the Vulkan instance
    vut_init_instance(ctx->name, &ctx->instance);

    // Let GLFW handle cross-platform surface creation
    glfwCreateWindowSurface(ctx->instance, ctx->window, NULL, &ctx->surface);

    int width, height;
    glfwGetFramebufferSize(ctx->window, &width, &height);
    ctx->window_extent.width = width;
    ctx->window_extent.height = height;

    // Select the most suitable gpu
    uint32_t gpu_count;
    vut_get_physical_devices(ctx->instance, &gpu_count, NULL);
    VkPhysicalDevice gpus[gpu_count];
    vut_get_physical_devices(ctx->instance, &gpu_count, gpus);
    vut_pick_physical_device(gpus, gpu_count, &ctx->gpu);

    // Get a queue which supports graphics and create a logical device
    vut_get_queue_family_indices(ctx->gpu, ctx->surface, &ctx->queue_family_count,
                                 &ctx->graphics_queue_family_index,
                                 &ctx->present_queue_family_index, &ctx->separate_present_queue);

    vut_init_device(ctx->gpu, ctx->graphics_queue_family_index, &ctx->device);

    // Store the correct queues from indices
    vkGetDeviceQueue(ctx->device, ctx->graphics_queue_family_index, 0, &ctx->graphics_queue);

    if (!ctx->separate_present_queue) {
        ctx->present_queue = ctx->graphics_queue;
    } else {
        vkGetDeviceQueue(ctx->device, ctx->present_queue_family_index, 0, &ctx->present_queue);
    }

    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting.
    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents.
    for (uint32_t i = 0; i < FRAME_LAG; i++) {
        vut_init_fence(ctx->device, &ctx->fences[i]);
        vut_init_semaphore(ctx->device, &ctx->image_acquired_semaphores[i]);
        vut_init_semaphore(ctx->device, &ctx->draw_complete_semaphores[i]);

        if (ctx->separate_present_queue) {
            vut_init_semaphore(ctx->device, &ctx->image_ownership_semaphores[i]);
        }
    }
}

void
vur_prepare_swapchain(VulkanContext* ctx)
{
    VkResult result;

    VkSurfaceCapabilitiesKHR capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->gpu, ctx->surface, &capabilities);
    if (result) {
        // Error
    }

    VkPresentModeKHR present_mode = vut_get_present_mode(capabilities, ctx->gpu, ctx->surface);
    VkExtent2D extent = vut_get_swapchain_extent(capabilities, ctx->window_extent);
    vut_get_surface_format(ctx->gpu, ctx->surface, &ctx->format, &ctx->color_space);

    vut_init_swapchain(ctx->gpu, ctx->device, ctx->surface, capabilities, extent, ctx->format,
                       present_mode, ctx->color_space, &ctx->swapchain);

    result =
        vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_image_count, NULL);
    if (result) {
        // Error
    }

    VkImage swapchain_images[ctx->swapchain_image_count];
    result = vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_image_count,
                                     swapchain_images);
    if (result) {
        // Error
    }

    ctx->swapchain_image_resources =
        malloc(sizeof(SwapchainImageResources) * ctx->swapchain_image_count);

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        ctx->swapchain_image_resources[i].image = swapchain_images[i];
        vut_init_image_view(ctx->device, ctx->format, ctx->swapchain_image_resources[i].image,
                            &ctx->swapchain_image_resources[i].view, false);
    }
}

void
vur_create_graphics_pipeline(VulkanContext* ctx)
{
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    vut_init_shader_module(ctx->device, "shaders/shader.vert.spv", &vert_shader_module);
    vut_init_shader_module(ctx->device, "shaders/shader.frag.spv", &frag_shader_module);

    const VkPipelineShaderStageCreateInfo vert_shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader_module,
        .pName = "main",
    };

    const VkPipelineShaderStageCreateInfo frag_shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader_module,
        .pName = "main",
    };

    const VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage,
                                                              frag_shader_stage };

    const VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL, // Optional
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL, // Optional
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)ctx->window_extent.width,
        .height = (float)ctx->window_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent.width = ctx->window_extent.width,
        .extent.height = ctx->window_extent.height,
    };

    const VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    const VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    const VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f,
    };

    vut_init_pipeline_layout(ctx->device, NULL, &ctx->pipeline_layout);
    vut_init_pipeline(ctx->device, shader_stages, &vertex_input, &input_assembly, &viewport_state,
                      &rasterizer, &multisampling, &color_blending, ctx->pipeline_layout,
                      ctx->render_pass, &ctx->pipeline);

    vkDestroyShaderModule(ctx->device, vert_shader_module, NULL);
    vkDestroyShaderModule(ctx->device, frag_shader_module, NULL);
}

void
vur_prepare_buffers(VulkanContext* ctx)
{
    vut_init_command_pool(ctx->device, ctx->graphics_queue_family_index, &ctx->command_pool);

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        vut_alloc_command_buffer(ctx->device, ctx->command_pool, 1,
                                 &ctx->swapchain_image_resources[i].command_buffer);
    }
}

void
vur_prepare(VulkanContext* ctx)
{
    // // Prepare GPU data
    // vur_prepare_depth(ctx);
    // vur_prepare_textures(ctx);
    // vur_prepare_meshes(ctx);

    // vut_init_descriptor_layout(ctx->device, &ctx->descriptor_layout);
    // vut_init_descriptor_pool(ctx->device, ctx->swapchain_image_count, &ctx->descriptor_pool);
    // vut_init_descriptor_set();

    vur_prepare_swapchain(ctx);
    vut_init_render_pass(ctx->device, ctx->format, &ctx->render_pass);
    vur_create_graphics_pipeline(ctx);

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        vut_init_framebuffer(ctx->device, ctx->render_pass, ctx->swapchain_image_resources[i].view,
                             ctx->window_extent, &ctx->swapchain_image_resources[i].framebuffer);
    }

    // Prepare the buffers that contain GPU data
    vur_prepare_buffers(ctx);
}

void
vur_record_buffers(VulkanContext* ctx)
{
    for (size_t i = 0; i < ctx->swapchain_image_count; i++) {
        vut_begin_command_buffer(ctx->swapchain_image_resources[i].command_buffer);
        vut_begin_render_pass(ctx->swapchain_image_resources[i].command_buffer, ctx->render_pass,
                              ctx->swapchain_image_resources[i].framebuffer, ctx->window_extent);

        // Bind pipeline to command buffer and specify its type (graphics or compute)
        vkCmdBindPipeline(ctx->swapchain_image_resources[i].command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipeline);

        // Record the pipeline in the command buffer
        vkCmdDraw(ctx->swapchain_image_resources[i].command_buffer, 3, 1, 0, 0);

        // Finishing up
        vkCmdEndRenderPass(ctx->swapchain_image_resources[i].command_buffer);

        if (vkEndCommandBuffer(ctx->swapchain_image_resources[i].command_buffer) != VK_SUCCESS) {
            // Error
        }
    }
}

// Forward Cast
void
vur_resize(VulkanContext* ctx);

// Main render loop
void
vur_draw(VulkanContext* ctx)
{
    // TODO: Move GLFW to own file
    glfwPollEvents();
    if (glfwWindowShouldClose(ctx->window)) {
        ctx->should_quit = true;
        return;
    }

    VkResult result;

    result = vkWaitForFences(ctx->device, 1, &ctx->fences[ctx->frame_index], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX,
                                   ctx->image_acquired_semaphores[ctx->frame_index], VK_NULL_HANDLE,
                                   &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        ctx->framebuffer_resized) {
        // Swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        ctx->framebuffer_resized = false;
        vur_resize(ctx);
        return;
    }

    VkSemaphore waitSemaphores[] = { ctx->image_acquired_semaphores[ctx->frame_index] };
    VkSemaphore signalSemaphores[] = { ctx->draw_complete_semaphores[ctx->frame_index] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &ctx->swapchain_image_resources[imageIndex].command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    result = vkResetFences(ctx->device, 1, &ctx->fences[ctx->frame_index]);

    if (vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, ctx->fences[ctx->frame_index]) !=
        VK_SUCCESS) {
        // Error
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { ctx->swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(ctx->present_queue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        ctx->framebuffer_resized) {
        ctx->framebuffer_resized = false;
        vur_resize(ctx);
        return;
    } else if (result != VK_SUCCESS) {
        // Error
    }

    ctx->frame_index = (ctx->frame_index + 1) % FRAME_LAG;
}

void
vur_destroy_pipeline(VulkanContext* ctx)
{
    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        vkDestroyFramebuffer(ctx->device, ctx->swapchain_image_resources[i].framebuffer, NULL);
    }

    vkDestroyPipeline(ctx->device, ctx->pipeline, NULL);
    vkDestroyPipelineLayout(ctx->device, ctx->pipeline_layout, NULL);
    vkDestroyRenderPass(ctx->device, ctx->render_pass, NULL);

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        vkDestroyImageView(ctx->device, ctx->swapchain_image_resources[i].view, NULL);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1,
                             &ctx->swapchain_image_resources[i].command_buffer);
        // vkDestroyBuffer(ctx->device, ctx->swapchain_image_resources[i].uniform_buffer, NULL);
        // vkFreeMemory(ctx->device, ctx->swapchain_image_resources[i].uniform_memory, NULL);
    }
    vkDestroyCommandPool(ctx->device, ctx->command_pool, NULL);
    free(ctx->swapchain_image_resources);
}

void
vur_resize(VulkanContext* ctx)
{
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(ctx->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(ctx->device);

    vur_destroy_pipeline(ctx);

    printf("Resizing window!\n");
    // Second, re-perform the vur_prepare() function, which will re-create the
    // swapchain:
    vur_prepare(ctx);
    vur_record_buffers(ctx);
}

void
vur_destroy(VulkanContext* ctx)
{
    vkDeviceWaitIdle(ctx->device);

    // Close any open window
    glfwTerminate();

    vur_destroy_pipeline(ctx);

    // Wait for fences from present operations
    for (uint32_t i = 0; i < FRAME_LAG; i++) {
        VkResult result = vkWaitForFences(ctx->device, 1, &ctx->fences[i], VK_TRUE, 1000);
        if (result == VK_TIMEOUT) {
            printf("Fences timed out. \n");
        }
        vkDestroyFence(ctx->device, ctx->fences[i], NULL);
        vkDestroySemaphore(ctx->device, ctx->image_acquired_semaphores[i], NULL);
        vkDestroySemaphore(ctx->device, ctx->draw_complete_semaphores[i], NULL);
        if (ctx->separate_present_queue) {
            vkDestroySemaphore(ctx->device, ctx->image_ownership_semaphores[i], NULL);
        }
    }

    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    vkDestroyDevice(ctx->device, NULL);
    vkDestroyInstance(ctx->instance, NULL);
}