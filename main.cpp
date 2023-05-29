#include "VulkanEngine.h"

int main() {
    VulkanAPI vkApp("Vulkan");
    try {
        VkGraphicsEngine app(&vkApp);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}