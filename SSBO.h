#ifndef hSSBO
#define hSSBO

#include <random>

struct pop {
    std::vector<Particle> particles;
    pop(int count) {
        particles.resize(count);
        std::random_device rand;
        std::mt19937_64 rndEngine(rand());
        for (auto& particle : particles) {
            std::uniform_real_distribution<float> rndDist(-0.79f, 0.79f);
            std::uniform_real_distribution<float> rndMass(0.01f, 0.25f);
            std::uniform_real_distribution<float> rndColor(0.1f, 1.0f);

            float x = rndDist(rndEngine);
            float y = rndDist(rndEngine);
            float z = rndDist(rndEngine);
            float w = rndMass(rndEngine);
            particle.position = glm::vec4(x, y, z, w);
            particle.velocity = glm::vec4(0);
            particle.color = glm::vec4(rndColor(rndEngine), rndColor(rndEngine), rndColor(rndEngine), 1.f);
        }
    }
};

namespace vk {
    struct SSBO : BufferObject, Descriptor {
        template<typename T>
        inline SSBO(T& bufferData, VkShaderStageFlags flags, uint32_t bindingCount = 2)
            : Descriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, flags, bindingCount)
        {
            loadData(bufferData);

            writeDescriptorSets(bindingCount);
        }
        ~SSBO() {
            std::for_each(std::execution::par, Buffer.begin(), Buffer.end(),
                [&](VkBuffer buffer) { vkDestroyBuffer(GPU::device, buffer, nullptr); });

            std::for_each(std::execution::par, Memory.begin(), Memory.end(),
                [&](VkDeviceMemory memory) { vkFreeMemory(GPU::device, memory, nullptr); });
        }
        void draw(int vertexCount = 1000) {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Buffer[SwapChain::currentFrame], offsets); // &SSBO.mBuffer
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    protected:
        template<typename T>
        inline void loadData(std::vector<T>& bufferData) {
            Buffer.resize(MAX_FRAMES_IN_FLIGHT);
            Memory.resize(MAX_FRAMES_IN_FLIGHT);

            bufferSize = sizeof(T) * bufferData.size();

            StageBuffer stageSSBO(bufferData.data(), bufferSize);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(Buffer[i], Memory[i],
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                stageSSBO.transferData(Buffer[i]);
            }
        }
    private:
        void writeDescriptorSets(uint32_t bindingCount) override {
            VkWriteDescriptorSet allocWrite
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            allocWrite.dstArrayElement = 0;
            allocWrite.descriptorCount = 1;
            allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount, allocWrite);

            VkDescriptorBufferInfo allocBuffer{};
            allocBuffer.offset = 0;
            allocBuffer.range = bufferSize;
            std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount, allocBuffer);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                bufferInfo[0].buffer = Buffer[(i - 1) % MAX_FRAMES_IN_FLIGHT];
                bufferInfo[1].buffer = Buffer[i];
                
                for (uint32_t j = 0; j < bindingCount; j++) {
                    descriptorWrites[j].dstSet = Sets[i];
                    descriptorWrites[j].dstBinding = j;
                    descriptorWrites[j].pBufferInfo = &bufferInfo[j];
                }
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };
}

#endif