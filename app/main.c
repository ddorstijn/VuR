#include "renderer.h"

int
main(int argc, char** argv)
{
    VulkanContext ctx;

    // Initialise the renderer
    vur_init_vulkan(&ctx, "VuR");
    vur_prepare_swapchain(&ctx);

    // Main render loop
    vur_draw(&ctx);

    // Clean up the renderer
    vur_destroy(&ctx);

    return 0;
}
