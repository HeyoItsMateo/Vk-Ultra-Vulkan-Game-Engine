#include "VulkanEngine.h"

int main() {
    VulkanAPI VkApp("Vulkan");
    try {
        VkGraphicsEngine app(&VkApp);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}