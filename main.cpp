#include "VulkanEngine.h"

#include <algorithm>

template<typename T> struct member;
template<typename Class, typename mType> struct member<mType Class::*>
{
    using type = mType;
};

template<auto mVar>
auto constexpr transformVector(auto& srcVec) {
    std::vector<typename member<decltype(mVar)>::type> dstVec(srcVec.size());
    std::transform(srcVec.begin(), srcVec.end(), dstVec.begin(), [=](auto const& mSrc) { return mSrc.*mVar; });
    return dstVec;
}

VkWindow window("Vulkan");
VkGraphicsEngine app(window);

UBO uniforms;
SSBO shaderStorage(VkSwapChain::Extent.height, VkSwapChain::Extent.width);

VkDataBuffer<UBO> ubo(uniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
VkDataBuffer<SSBO> ssbo(shaderStorage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

VkTexture planks("textures/planks.png");
VkTextureSet textureSet(planks);

// TODO: Determine how to extract the Sets and layouts members from the members of "descriptors"
// and place into the pipeline object


VkDescriptorSet sets[] = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };

std::vector<VkDescriptor> descriptors = { ubo, textureSet, ssbo };
std::vector<VkDescriptorSetLayout> layouts = { ubo.SetLayout, textureSet.SetLayout, ssbo.SetLayout };

std::vector<VkDescriptorSetLayout> test1 = transformVector<&VkDescriptor::SetLayout>(descriptors);

VkShader vertShader("shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader fragShader("shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
VkShader compShader("shaders/comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

//std::vector<VkShader> shadersTest = { vertShader, fragShader };

std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShader.stageInfo, fragShader.stageInfo };

//std::vector<VkPipelineShaderStageCreateInfo> shaderStages = transformVector<&VkShader::stageInfo>(shadersTest);


VkGraphicsPipeline pipeline(test1, shaderStages);



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