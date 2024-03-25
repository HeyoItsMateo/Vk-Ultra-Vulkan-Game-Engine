#pragma once
#ifndef hSSBO
#define hSSBO

#include "vk.gpu.h"
#include "vk.buffers.h"
#include "descriptors.h"

namespace vk {
    struct SSBO : DataBuffer, Descriptor {
        template<typename T>
        inline SSBO(T& bufferData, VkShaderStageFlags flags, VkBufferUsageFlags usage, uint32_t bindingCount = 2);
    public:
        void draw();
    private:
        int vertexCount = 0;
        void writeDescriptorSets(uint32_t bindingCount) override;
    };
}

#include "vk.ssbo.ipp"

#endif