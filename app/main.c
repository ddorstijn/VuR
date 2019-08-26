#include "renderer.h"

int
main(int argc, char** argv)
{
    VulkanContext ctx;

    // Initialise the renderer
    vur_init_vulkan(&ctx, "VuR");
    vur_prepare(&ctx);
    // Make a command list of all the draws that need to be done
    vur_record_buffers(&ctx);

    // Main render loop
    while (!ctx.should_quit) {
        vur_draw(&ctx);
    }

    // Clean up the renderer
    vur_destroy(&ctx);

    return 0;
}
