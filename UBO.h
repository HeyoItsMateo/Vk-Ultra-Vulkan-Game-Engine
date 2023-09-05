#ifndef hUBO
#define hUBO

namespace vk {
    struct Uniforms {
        double dt = 1.0;
        alignas(16) glm::mat4 model = glm::mat4(1.f);
        Camera camera;
        void update(double lastTime, float FOVdeg = 45.f, float nearPlane = 0.01f, float farPlane = 1000.f) {
            std::jthread t1([this]
                { modelUpdate(); }
            );
            std::jthread t2([this, FOVdeg, nearPlane, farPlane]
                { camera.update(FOVdeg, nearPlane, farPlane); }
            );
            std::jthread t3([this, lastTime]
                { deltaTime(lastTime); }
            );
        }
    private:
        void modelUpdate() {
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        void deltaTime(double lastTime) {
            double currentTime = glfwGetTime();
            dt = (currentTime - lastTime);
        }
    };

    struct UBO : BufferObject, Descriptor {
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        template<typename T>
        inline UBO(T& uniforms, VkShaderStageFlags flag, uint32_t bindingCount = 1)
            : Descriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, flag, bindingCount)
        {
            loadData(uniforms);

            writeDescriptorSets(bindingCount);
        }
        ~UBO() {
            vkUnmapMemory(GPU::device, stagingBufferMemory);
            vkDestroyBuffer(GPU::device, stagingBuffer, nullptr);
            vkFreeMemory(GPU::device, stagingBufferMemory, nullptr);

            std::for_each(std::execution::par, Buffer.begin(), Buffer.end(),
                [&](VkBuffer buffer) { vkDestroyBuffer(GPU::device, buffer, nullptr); });

            std::for_each(std::execution::par, Memory.begin(), Memory.end(),
                [&](VkDeviceMemory memory) { vkFreeMemory(GPU::device, memory, nullptr); });
        }

        template <typename T>
        inline void update(T& uniforms) {
            uniforms.update(vk::SwapChain::lastTime);

            memcpy(data, &uniforms, (size_t)bufferSize);

            copyBuffer(stagingBuffer, Buffer[vk::SwapChain::currentFrame]);
        }
    protected:
        template<typename T>
        inline void loadData(T& UBO) {
            Buffer.resize(MAX_FRAMES_IN_FLIGHT);
            Memory.resize(MAX_FRAMES_IN_FLIGHT);

            bufferSize = sizeof(T);

            createBuffer(stagingBuffer, stagingBufferMemory,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            vkMapMemory(GPU::device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, &UBO, (size_t)bufferSize);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(Buffer[i], Memory[i],
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                copyBuffer(stagingBuffer, Buffer[i]);
            }
        }
    private:
        void* data;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        void writeDescriptorSets(uint32_t bindingCount) {
            std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount);
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                bufferInfo[0].buffer = Buffer[i];

                for (uint32_t j = 0; j < bindingCount; j++) {
                    bufferInfo[j].offset = 0;
                    bufferInfo[j].range = bufferSize;
                    descriptorWrites[j] = writeSet(bufferInfo, { i, j });
                }
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };
}

#endif