#include "vk.ubo.h"
namespace vk {
    template<typename T>
    inline UBO::UBO(T& uniforms, VkShaderStageFlags flag, uint32_t bindingCount)
        : DataBuffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        Descriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, flag, bindingCount), stageUBO(&uniforms, sizeof(T))
    {
        stageUBO.transferData(buffers);
        writeDescriptorSets(bindingCount);
    }

    template<typename T>
    inline void UBO::update(T& uniforms) {
        uniforms.update();
        stageUBO.update(&uniforms, buffers);
    }

    void UBO::writeDescriptorSets(uint32_t bindingCount) {
        VkWriteDescriptorSet allocWrite
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        allocWrite.dstArrayElement = 0;
        allocWrite.descriptorCount = 1;
        allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        std::vector<VkWriteDescriptorSet> descriptorWrites(MAX_FRAMES_IN_FLIGHT, allocWrite);

        VkDescriptorBufferInfo allocBuffer{};
        allocBuffer.offset = 0;
        allocBuffer.range = size;
        std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount, allocBuffer);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            descriptorWrites[i].dstSet = Sets[i];
            for (uint32_t j = 0; j < bindingCount; j++) {
                bufferInfo[j].buffer = buffers[i];
                descriptorWrites[j].dstBinding = j;
                descriptorWrites[j].pBufferInfo = &bufferInfo[j];
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    }
}