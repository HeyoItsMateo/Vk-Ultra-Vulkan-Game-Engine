#pragma once
#ifndef hCompute
#define hCompute

#include "vk.swapchain.h"
#include "vk.cpu.h"

#include "vk.pipeline.h"
#include "vk.shader.h"

namespace vk {
    struct Workgroup {
        uint32_t x, y, z;
    };

    struct ComputePPL : Pipeline {
        Workgroup workgroup;
        ComputePPL(Shader const& computeShader, std::vector<VkDescriptorSet>& descSets, std::vector<VkDescriptorSetLayout>& setLayouts, Workgroup workgroups = { 10, 10, 10 });
    public:
        void dispatch();
    private:
        void vkCreatePipeline(Shader const& computeShader);
    };
}

#endif