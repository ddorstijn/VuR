#include "renderer.h"

int
main(int argc, char** argv)
{
    VulkanContext ctx;

    // Initialise the renderer
    vur_init(&ctx, "VuR");

    // Main render loop
    while (!ctx.should_quit) {
        // Get events from window
        vur_update_window(&ctx);

        if (ctx.should_quit) {
            break;
        }

        // Update game logic

        // Renderer draw
        vur_draw(&ctx);
    }

    // Clean up the renderer
    vur_destroy(&ctx);

    return 0;
}
