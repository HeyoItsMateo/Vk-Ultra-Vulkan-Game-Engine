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
            std::uniform_real_distribution<float> rndColor(0.1f, 1.0f);

            float x = rndDist(rndEngine);
            float y = rndDist(rndEngine);
            float z = rndDist(rndEngine);
            particle.position = glm::vec4(x, y, z, 1.f);
            particle.velocity = glm::vec4(0);
            particle.color = glm::vec4(rndColor(rndEngine), rndColor(rndEngine), rndColor(rndEngine), 1.f);
        }
    }
};

namespace vk {
    struct SSBO : BufferObject, Descriptor {
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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
        void draw(int vertexCount) {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Buffer[SwapChain::currentFrame], offsets); // &SSBO.mBuffer
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    protected:
        template<typename T>
        inline void loadData(std::vector<T>& bufferData) {
            void* data;
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;

            Buffer.resize(MAX_FRAMES_IN_FLIGHT);
            Memory.resize(MAX_FRAMES_IN_FLIGHT);

            bufferSize = sizeof(T) * bufferData.size();

            createBuffer(stagingBuffer, stagingBufferMemory,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            vkMapMemory(GPU::device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, bufferData.data(), (size_t)bufferSize);
            vkUnmapMemory(GPU::device, stagingBufferMemory);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(Buffer[i], Memory[i],
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                copyBuffer(stagingBuffer, Buffer[i]);
            }

            vkDestroyBuffer(GPU::device, stagingBuffer, nullptr);
            vkFreeMemory(GPU::device, stagingBufferMemory, nullptr);
        }
    private:
        void writeDescriptorSets(uint32_t bindingCount) {
            std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount);
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                bufferInfo[0].buffer = Buffer[(i - 1) % MAX_FRAMES_IN_FLIGHT];
                bufferInfo[1].buffer = Buffer[i];

                for (uint32_t j = 0; j < bindingCount; j++) {
                    bufferInfo[j].offset = 0;
                    bufferInfo[j].range = bufferSize;

                    descriptorWrites[j] = writeSet(bufferInfo, { i, j });
                }
                vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
            }
        }
    };
}

#endif