#include "renderer.h"

int
main(int argc, char** argv)
{
    VulkanContext* ctx;
    ctx = vur_init_vulkan("VuR");

    // Main render loop
    vur_draw(ctx);

    vur_destroy(ctx);

    return 0;
}
