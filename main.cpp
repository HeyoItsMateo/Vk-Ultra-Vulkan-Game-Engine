#include "VulkanEngine.h"



int main() {

    VkWindow window("Vulkan");
    VkGraphicsEngine app(window);
    VkGraphicsPipeline pipeline(app);
    try {
        app.run(pipeline);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}