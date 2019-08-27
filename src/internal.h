#ifndef INTERNAL_H
#define INTERNAL_H

typedef struct _VulkanContext VulkanContext;

// Init

/**
 * @brief GLFW window creation wrapper
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_setup_window(VulkanContext* ctx);

/**
 * @brief Get size from GLFW and update context
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_update_window_size(VulkanContext* ctx);

/**
 * @brief Initialize instance and device
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_init_vulkan(VulkanContext* ctx);

/**
 * @brief Get the GPU
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_pick_physical_device(VulkanContext* ctx);

/**
 * @brief Create a vulkan device
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_create_device(VulkanContext* ctx);

/**
 * @brief Create semaphores and fences
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_setup_synchronization(VulkanContext* ctx);

// Prepare

/**
 * @brief Wrapper function for the different preparations for the pipeline
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_prepare(VulkanContext* ctx);

/**
 * @brief Creation of the swapchain
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_prepare_swapchain(VulkanContext* ctx);

/**
 * @brief Create all the image views for the swapchain images
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_prepare_image_views(VulkanContext* ctx);

/**
 * @brief Create command buffers
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_prepare_buffers(VulkanContext* ctx);

/**
 * @brief Create grpahics pipeline
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_prepare_pipeline(VulkanContext* ctx);

// Draw

/**
 * @brief Record the drawing process in the buffers
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_record_buffers(VulkanContext* ctx);

// Destroy

/**
 * @brief Recreate the whole pipeline on window size change
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_resize(VulkanContext* ctx);

/**
 * @brief Destroy the pipeline for resizing
 *
 * @param[in] ctx VulkanContext handle
 */
void
vur_destroy_pipeline(VulkanContext* ctx);

#endif // INTERNAL_H