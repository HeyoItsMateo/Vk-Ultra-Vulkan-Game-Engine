#include "VulkanEngine.h"

#include <algorithm>
#include <functional>

VkWindow window("Vulkan");
VkGraphicsEngine app(window);

UBO uniforms;
SSBO shaderStorage;

VkUniformBuffer<UBO> ubo(uniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_COMPUTE_BIT);
VkStorageBuffer ssbo(shaderStorage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

VkTexture planks("textures/planks.png");
VkTextureSet textureSet(planks);

VkDescriptorSet sets[] = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };

std::vector<VkDescriptor*> descriptors = { &ubo, &textureSet, &ssbo };

VkShader vertShader("shaders/vertexVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader fragShader("shaders/vertexFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
VkShader compShader("shaders/pointComp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

VkShader pointVert("shaders/pointVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader pointFrag("shaders/pointFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

std::vector<VkShader*> shaders = { &vertShader, &fragShader };
std::vector<VkShader*> compShaders = { &pointVert, &pointFrag};

VkGraphicsPipeline<Vertex> pipeline(descriptors, shaders);

VkTestPipeline<Particle> ptclPipeline(descriptors, compShaders);

VkComputePipeline testPpln(descriptors, compShader.stageInfo);


struct s_doer {
    void operator()(int n) {
        std::cout << n << "\n";
    };
    //std::function<void()> func;
    template<typename F>
    friend void operator<<(s_doer&&, F&& func) {
        std::cout << "test" << std::endl;
        func();
        std::cout << "test" << std::endl;
    }
};

struct s_informer {
    s_informer(int n) {
        info = n;
    }
    void inform(int n) {
        std::cout << n << "\n";
    }
private:
    int info;
    template<typename F>
    friend void operator<<(s_doer&&, F&& func);
};



s_informer informer(4);

int main() {
    int n = 2;
    s_doer{} << [n] {informer.inform(n); };

    try {
        app.run(pipeline, ptclPipeline, testPpln, ssbo, uniforms, ubo, 3, sets);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}