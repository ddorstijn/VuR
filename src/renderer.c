#include "renderer.h"

#include "vk_util.h"
#include <stdlib.h>

void
vur_init_vulkan(VulkanContext* ctx, const char* app_name)
{
    // Make sure the whole struct is NULL
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
    vut_get_queue_family_indices(ctx->gpu,
                                 ctx->surface,
                                 &ctx->queue_family_count,
                                 &ctx->graphics_queue_family_index,
                                 &ctx->present_queue_family_index,
                                 &ctx->separate_present_queue);

    vut_init_device(ctx->gpu, ctx->graphics_queue_family_index, &ctx->device);

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
        vut_init_fence(ctx->device, &ctx->fences[i]);
        vut_init_semaphore(ctx->device, &ctx->image_acquired_semaphores[i]);
        vut_init_semaphore(ctx->device, &ctx->draw_complete_semaphores[i]);

        if (ctx->separate_present_queue) {
            vut_init_semaphore(ctx->device, &ctx->image_ownership_semaphores[i]);
        }
    }
}

void
vur_prepare(VulkanContext* ctx)
{
    if (ctx->command_pool == VK_NULL_HANDLE) {
        vut_init_command_pool(ctx->device, ctx->graphics_queue_family_index, &ctx->command_pool);
    }

    vut_alloc_command_buffer(ctx->device, ctx->command_pool, &ctx->command_buffer);
    vut_begin_command_buffer(ctx->command_buffer);

    vur_prepare_buffers(ctx);

    vur_prepare_depth(ctx);
    vur_prepare_textures(ctx);
    vur_prepare_meshes(ctx);

    vut_init_descriptor_layout(ctx->device, &ctx->descriptor_layout);
    vut_init_pipeline_layout(ctx->device, &ctx->descriptor_layout, &ctx->pipeline_layout);
    vut_init_render_pass(ctx->device, ctx->format, ctx->depth.format, &ctx->render_pass);
    vur_prepare_pipeline(ctx);

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        vut_alloc_command_buffer(
          ctx->device, ctx->command_pool, &ctx->swapchain_image_resources[i].cmd);
    }

    if (ctx->separate_present_queue) {
        vut_init_command_pool(
          ctx->device, ctx->present_queue_family_index, &ctx->present_command_pool);

        for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
            vut_alloc_command_buffer(ctx->device,
                                     ctx->present_command_pool,
                                     &ctx->swapchain_image_resources[i].graphics_to_present_cmd);

            vut_build_image_ownership_cmd(ctx->swapchain_image_resources[i].graphics_to_present_cmd,
                                          ctx->graphics_queue_family_index,
                                          ctx->present_queue_family_index,
                                          ctx->swapchain_image_resources[i].image);
        }
    }

    vut_init_descriptor_pool(ctx->device, ctx->swapchain_image_count, &ctx->descriptor_pool);
    vut_init_descriptor_set();

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        vut_init_framebuffer(ctx->device,
                             ctx->render_pass,
                             ctx->depth.view,
                             ctx->swapchain_image_resources[i].view,
                             ctx->width,
                             ctx->height,
                             &ctx->swapchain_image_resources[i].framebuffer);
    }
}

void
vur_prepare_buffers(VulkanContext* ctx)
{
    VkResult result;

    result =
      vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_image_count, NULL);
    if (result) {
        // Error
    }

    VkImage swapchain_images[ctx->swapchain_image_count];
    result = vkGetSwapchainImagesKHR(
      ctx->device, ctx->swapchain, &ctx->swapchain_image_count, swapchain_images);
    if (result) {
        // Error
    }

    ctx->swapchain_image_resources = (SwapchainImageResources*)malloc(
      sizeof(SwapchainImageResources) * ctx->swapchain_image_count);

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        ctx->swapchain_image_resources[i].image = swapchain_images[i];
        vut_init_image_view(ctx->device,
                            ctx->format,
                            ctx->swapchain_image_resources[i].image,
                            &ctx->swapchain_image_resources[i].view,
                            false);
    }
}

void
vur_prepare_depth(VulkanContext* ctx)
{
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    vut_init_image(ctx->device, depth_format, ctx->width, ctx->height, &ctx->depth.image);

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(ctx->device, ctx->depth.image, &mem_reqs);

    ctx->depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ctx->depth.mem_alloc.pNext = NULL;
    ctx->depth.mem_alloc.allocationSize = mem_reqs.size;
    ctx->depth.mem_alloc.memoryTypeIndex = 0;

    /* allocate memory */
    VkResult result = vkAllocateMemory(ctx->device, &ctx->depth.mem_alloc, NULL, &ctx->depth.mem);
    if (result) {
        // Error
    }
    /* bind memory */
    result = vkBindImageMemory(ctx->device, ctx->depth.image, ctx->depth.mem, 0);
    if (result) {
        // Error
    }

    vut_init_image_view(ctx->device, depth_format, ctx->depth.image, &ctx->depth.view, true);
}

void
vur_prepare_textures(VulkanContext* ctx)
{
    // const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    // VkFormatProperties props;
    // uint32_t i;

    // vkGetPhysicalDeviceFormatProperties(ctx->gpu, tex_format, &props);

    // for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
    //     VkResult result;

    //     if ((props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
    //     !demo->use_staging_buffer) {
    //         /* Device can texture using linear textures */
    //         demo_prepare_texture_image(
    //             demo, tex_files[i], &demo->textures[i], VK_IMAGE_TILING_LINEAR,
    //             VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    //             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    //         // Nothing in the pipeline needs to be complete to start, and don't allow fragment
    //         // shader to run until layout transition completes
    //         demo_set_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
    //                               VK_IMAGE_LAYOUT_PREINITIALIZED, demo->textures[i].imageLayout,
    //                               0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    //         demo->staging_texture.image = 0;
    //     } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
    //         /* Must use staging buffer to copy linear texture to optimized */

    //         memset(&demo->staging_texture, 0, sizeof(demo->staging_texture));
    //         demo_prepare_texture_buffer(demo, tex_files[i], &demo->staging_texture);

    //         demo_prepare_texture_image(demo, tex_files[i], &demo->textures[i],
    //         VK_IMAGE_TILING_OPTIMAL,
    //                                    (VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    //                                    VK_IMAGE_USAGE_SAMPLED_BIT),
    //                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //         demo_set_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
    //                               VK_IMAGE_LAYOUT_PREINITIALIZED,
    //                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
    //                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //                               VK_PIPELINE_STAGE_TRANSFER_BIT);

    //         VkBufferImageCopy copy_region = {
    //             .bufferOffset = 0,
    //             .bufferRowLength = demo->staging_texture.tex_width,
    //             .bufferImageHeight = demo->staging_texture.tex_height,
    //             .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
    //             .imageOffset = { 0, 0, 0 },
    //             .imageExtent = { demo->staging_texture.tex_width,
    //             demo->staging_texture.tex_height, 1 },
    //         };

    //         vkCmdCopyBufferToImage(demo->cmd, demo->staging_texture.buffer,
    //         demo->textures[i].image,
    //                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    //         demo_set_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
    //                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //                               demo->textures[i].imageLayout, VK_ACCESS_TRANSFER_WRITE_BIT,
    //                               VK_PIPELINE_STAGE_TRANSFER_BIT,
    //                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    //     } else {
    //         // Error
    //     }

    //     const VkSamplerCreateInfo sampler = {
    //         .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    //         .pNext = NULL,
    //         .magFilter = VK_FILTER_NEAREST,
    //         .minFilter = VK_FILTER_NEAREST,
    //         .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    //         .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    //         .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    //         .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    //         .mipLodBias = 0.0f,
    //         .anisotropyEnable = VK_FALSE,
    //         .maxAnisotropy = 1,
    //         .compareOp = VK_COMPARE_OP_NEVER,
    //         .minLod = 0.0f,
    //         .maxLod = 0.0f,
    //         .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    //         .unnormalizedCoordinates = VK_FALSE,
    //     };

    //     VkImageViewCreateInfo view = {
    //         .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    //         .pNext = NULL,
    //         .image = VK_NULL_HANDLE,
    //         .viewType = VK_IMAGE_VIEW_TYPE_2D,
    //         .format = tex_format,
    //         .components =
    //             {
    //                 VK_COMPONENT_SWIZZLE_R,
    //                 VK_COMPONENT_SWIZZLE_G,
    //                 VK_COMPONENT_SWIZZLE_B,
    //                 VK_COMPONENT_SWIZZLE_A,
    //             },
    //         .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    //         .flags = 0,
    //     };

    //     /* create sampler */
    //     err = vkCreateSampler(demo->device, &sampler, NULL, &demo->textures[i].sampler);
    //     assert(!err);

    //     /* create image view */
    //     view.image = demo->textures[i].image;
    //     err = vkCreateImageView(demo->device, &view, NULL, &demo->textures[i].view);
    //     assert(!err);
    // }
}

void
vur_prepare_meshes(VulkanContext* ctx)
{}

void
vur_prepare_pipeline(VulkanContext* ctx)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkResult result;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = ctx->pipeline_layout;

    memset(&vi, 0, sizeof(vi));
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    vur_prepare_vs(ctx);
    vur_prepare_fs(ctx);

    // Two stages: vs and fs
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = ctx->vert_shader_module;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = ctx->frag_shader_module;
    shaderStages[1].pName = "main";

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    result = vkCreatePipelineCache(ctx->device, &pipelineCache, NULL, &ctx->pipeline_cache);
    if (result) {
        // Error
    }

    pipeline.pVertexInputState = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pMultisampleState = &ms;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.stageCount = ARRAY_SIZE(shaderStages);
    pipeline.pStages = shaderStages;
    pipeline.renderPass = ctx->render_pass;
    pipeline.pDynamicState = &dynamicState;

    pipeline.renderPass = ctx->render_pass;

    result = vkCreateGraphicsPipelines(
      ctx->device, ctx->pipeline_cache, 1, &pipeline, NULL, &ctx->pipeline);
    if (result) {
        // Error
    }

    vkDestroyShaderModule(ctx->device, ctx->frag_shader_module, NULL);
    vkDestroyShaderModule(ctx->device, ctx->vert_shader_module, NULL);
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