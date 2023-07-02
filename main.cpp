#include "VulkanEngine.h"

VkWindow window("Vulkan");
VkGraphicsEngine app(window);

UBO uniforms;
SSBO shaderStorage(VkSwapChain::Extent.height, VkSwapChain::Extent.width);

VkDataBuffer<UBO> ubo(uniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
VkDataBuffer<SSBO> ssbo(shaderStorage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

VkTexture planks("textures/planks.png");
VkTextureSet textureSet(planks);

std::vector<VkDescriptor> descriptors = { ubo, ssbo, textureSet };

VkDescriptorSet sets[] = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };

std::vector<VkDescriptorSetLayout> layouts = { ubo.SetLayout, textureSet.SetLayout, ssbo.SetLayout };

VkGraphicsPipeline pipeline(layouts);

int main() {
    try {
        app.run(pipeline, uniforms, ubo, 3, sets);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}