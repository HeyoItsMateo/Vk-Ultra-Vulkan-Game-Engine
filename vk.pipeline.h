#ifndef hPipeline
#define hPipeline

#include "vk.swapchain.h"
#include "vk.shader.h"
#include "vk.cpu.h"

namespace vk {
    struct Pipeline {
        ~Pipeline();
        virtual void bind();
    public:
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> sets;
    protected:
        VkPipelineBindPoint bindPoint{};
        static void vkLoadSetLayout(std::vector<VkDescriptorSetLayout>& SetLayout, VkPipelineLayout& layout);

        virtual std::vector<VkPipelineShaderStageCreateInfo> stageInfo(std::vector<Shader>& shaders);

        static VkPipelineViewportStateCreateInfo viewportState(uint32_t viewportCount, uint32_t scissorCount, VkViewport* pViewports = nullptr, VkRect2D* pScissors = nullptr);
        static VkPipelineRasterizationStateCreateInfo rasterState(VkPolygonMode drawType, VkCullModeFlags cullType = VK_CULL_MODE_BACK_BIT, VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE);
        static VkPipelineMultisampleStateCreateInfo msaaState(bool sampleShading, float minSampleShading);
        static VkPipelineDepthStencilStateCreateInfo depthStencilState();
        static VkPipelineColorBlendStateCreateInfo colorBlendState(VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkBool32 logicOpEnable, VkLogicOp logicOp = VK_LOGIC_OP_COPY);
        static VkPipelineDynamicStateCreateInfo dynamicState(std::vector<VkDynamicState>& dynamicStates);
    };
}
#endif