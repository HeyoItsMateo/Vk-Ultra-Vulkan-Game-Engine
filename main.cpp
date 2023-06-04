#include "VulkanEngine.h"

int main() {
    VkWindow window("Vulkan");
    try {
        VkGraphicsEngine app(&window);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}