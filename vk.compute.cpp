#include "vk.compute.h"

namespace vk {
    ComputePPL::ComputePPL(Shader const& computeShader, std::vector<VkDescriptorSet>& descSets, std::vector<VkDescriptorSetLayout>& setLayouts, Workgroup workgroups)
        :workgroup(workgroups)
    {
        bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        sets = descSets;
        vkLoadSetLayout(setLayouts, layout);
        vkCreatePipeline(computeShader);
    }

    void ComputePPL::dispatch() {
        VkCommandBuffer& commandBuffer = EngineCPU::computeCommands[SwapChain::currentFrame];

        vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
        vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
        vkCmdDispatch(commandBuffer, workgroup.x, workgroup.y, workgroup.z);
    }

    void ComputePPL::vkCreatePipeline(Shader const& computeShader) {
        VkPipelineShaderStageCreateInfo stageInfo
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stageInfo.module = computeShader.shaderModule;
        stageInfo.stage = computeShader.shaderStage;
        stageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo
        { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        pipelineInfo.layout = layout;
        pipelineInfo.stage = stageInfo;

        VK_CHECK_RESULT(vkCreateComputePipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
    }
}
