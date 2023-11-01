#pragma once
namespace vk {
    template<typename T>
    inline SSBO::SSBO(T& content, VkShaderStageFlags flags, VkBufferUsageFlags usage, uint32_t bindingCount)
        : Descriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, flags, bindingCount),
        DataBuffer(sizeof(T) * content.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        vertexCount = content.size();
        size = sizeof(T) * content.size();

        StageBuffer_ stageSSBO(content.data(), size);
        stageSSBO.transferData(buffers);

        writeDescriptorSets(bindingCount);
    }
    /* Public */
    void SSBO::draw() {
        VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

        VkDeviceSize offsets[] = { 0, 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers.data(), offsets); // &SSBO.mBuffer
        vkCmdDraw(commandBuffer, 1, vertexCount, 0, 0);
    }
    /* Private */
    void SSBO::writeDescriptorSets(uint32_t bindingCount)
    {
        VkWriteDescriptorSet allocWrite
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        allocWrite.dstArrayElement = 0;
        allocWrite.descriptorCount = 1;
        allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount, allocWrite);

        VkDescriptorBufferInfo allocBuffer{};
        allocBuffer.offset = 0;
        allocBuffer.range = size;
        std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount, allocBuffer);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            bufferInfo[0].buffer = buffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
            bufferInfo[1].buffer = buffers[i];

            for (uint32_t j = 0; j < bindingCount; j++) {
                descriptorWrites[j].dstSet = Sets[i];
                descriptorWrites[j].dstBinding = j;
                descriptorWrites[j].pBufferInfo = &bufferInfo[j];
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }

    }
}