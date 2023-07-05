#include "VulkanEngine.h"

#include <algorithm>

VkWindow window("Vulkan");
VkGraphicsEngine app(window);

UBO uniforms;
SSBO shaderStorage(VkSwapChain::Extent.height, VkSwapChain::Extent.width);

VkDataBuffer<UBO> ubo(uniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
VkDataBuffer<SSBO> ssbo(shaderStorage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

VkTexture planks("textures/planks.png");
VkTextureSet textureSet(planks);

VkDescriptorSet sets[] = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };

std::vector<VkDescriptor*> descriptors = { &ubo, &textureSet, &ssbo };

VkShader vertShader("shaders/vertexVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader fragShader("shaders/vertexFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
VkShader compShader("shaders/pointComp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

std::vector<VkShader*> shaders = { &vertShader, &fragShader };

VkGraphicsPipeline pipeline(descriptors, shaders);

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