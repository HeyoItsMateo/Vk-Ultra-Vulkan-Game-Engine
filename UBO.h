#ifndef hUBO
#define hUBO

namespace vk {
    struct Uniforms {
        double dt = 1.0;
        alignas(16) glm::mat4 model = glm::mat4(1.f);
        Camera camera;
        void update(float FOVdeg = 45.f, float nearPlane = 0.01f, float farPlane = 1000.f) {
            std::jthread t1([&] { modelUpdate(); });
            std::jthread t2([&] { camera.update(FOVdeg, nearPlane, farPlane); });
            std::jthread t3([&] { deltaTime(); });
        }
    private:
        void modelUpdate() {
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        void deltaTime() {
            double currentTime = glfwGetTime();
            dt = (currentTime - SwapChain::lastTime);
        }
    };

    struct UBO : BufferObject, Descriptor {
        template<typename T>
        inline UBO(T& uniforms, VkShaderStageFlags flag, uint32_t bindingCount = 1)
            : Descriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, flag, bindingCount), stageUBO(&uniforms, sizeof(T))
        {
            bufferSize = sizeof(T);
            Buffer.resize(MAX_FRAMES_IN_FLIGHT);
            Memory.resize(MAX_FRAMES_IN_FLIGHT);

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(Buffer[i], Memory[i],
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                stageUBO.transferData(Buffer[i]);
            }
            writeDescriptorSets(bindingCount);
        }
        ~UBO() {
            std::for_each(std::execution::par, Buffer.begin(), Buffer.end(),
                [&](VkBuffer buffer) { vkDestroyBuffer(GPU::device, buffer, nullptr); });

            std::for_each(std::execution::par, Memory.begin(), Memory.end(),
                [&](VkDeviceMemory memory) { vkFreeMemory(GPU::device, memory, nullptr); });
        }
        template <typename T>
        inline void update(T& uniforms) {
            uniforms.update();

            stageUBO.update(&uniforms, Buffer[SwapChain::currentFrame]);
        }
    private:
        StageBuffer stageUBO;
        void writeDescriptorSets(uint32_t bindingCount) override {
            VkWriteDescriptorSet allocWrite
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            allocWrite.dstArrayElement = 0;
            allocWrite.descriptorCount = 1;
            allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount, allocWrite);

            VkDescriptorBufferInfo allocBuffer{};
            allocBuffer.offset = 0;
            allocBuffer.range = bufferSize;
            std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount, allocBuffer);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                descriptorWrites[i].dstSet = Sets[i];
                for (uint32_t j = 0; j < bindingCount; j++) {
                    bufferInfo[j].buffer = Buffer[i];
                    descriptorWrites[j].dstBinding = j;
                    descriptorWrites[j].pBufferInfo = &bufferInfo[j];
                }
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };
}

#endif