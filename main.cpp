#include "VulkanEngine.h"

int main() {
    VulkanAPI VkApplication("Vulkan");
    try {
        VkGraphicsEngine app(&VkApplication);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}