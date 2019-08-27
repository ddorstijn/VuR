#ifndef INTERNAL_H
#define INTERNAL_H

// Init
typedef struct _VulkanContext VulkanContext;

void
vur_setup_window(VulkanContext* ctx);

void
vur_update_window_size(VulkanContext* ctx);

void
vur_init_vulkan(VulkanContext* ctx);

void
vur_pick_physical_device(VulkanContext* ctx);

void
vur_create_device(VulkanContext* ctx);

void
vur_setup_synchronization(VulkanContext* ctx);

void
vur_prepare(VulkanContext* ctx);

void
vur_prepare_swapchain(VulkanContext* ctx);

void
vur_prepare_image_views(VulkanContext* ctx);

void
vur_prepare_buffers(VulkanContext* ctx);

void
vur_prepare_pipeline(VulkanContext* ctx);

void
vur_record_buffers(VulkanContext* ctx);

void
vur_resize(VulkanContext* ctx);

void
vur_destroy_pipeline(VulkanContext* ctx);

#endif // INTERNAL_H