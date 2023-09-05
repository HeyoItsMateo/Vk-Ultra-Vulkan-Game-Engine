#ifndef hBuffers
#define hBuffers

namespace vk {
    struct BufferObject : Command {
    public:
        std::vector<VkBuffer> Buffer;
    protected:
        VkDeviceSize bufferSize;
        std::vector<VkDeviceMemory> Memory;

        void createBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
            VkBufferCreateInfo bufferInfo
            { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = bufferSize;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(GPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }

            createMemory(buffer, memory, properties);
        }
        void createMemory(VkBuffer& buffer, VkDeviceMemory& memory, VkMemoryPropertyFlags properties) {
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(GPU::device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(GPU::device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(GPU::device, buffer, memory, 0);
        }
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer) {
            beginCommand();

            VkBufferCopy copyRegion{};
            copyRegion.size = bufferSize;
            vkCmdCopyBuffer(Command::buffer, srcBuffer, dstBuffer, 1, &copyRegion);

            endCommand();
        }
    };

    struct Buffer {
        VkBuffer self;
        VkDeviceSize size;
        VkDeviceMemory memory;
    protected:
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
        void createBuffer() {
            VkBufferCreateInfo bufferInfo
            { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(GPU::device, &bufferInfo, nullptr, &self) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }
        }
        void allocateMemory() {
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(GPU::device, self, &memRequirements);

            VkMemoryAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(GPU::device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(GPU::device, self, memory, 0);
        }
    };
    struct StageBuffer : Buffer {
        StageBuffer() {
            std::jthread t1([&] {usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; });
            std::jthread t2([&] {properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; });
        }
        void init(VkDeviceSize inSize) {
            size = inSize;
            createBuffer();
            allocateMemory();
        }
        void stageData(const void* content) {
            vkMapMemory(GPU::device, memory, 0, size, 0, &data);
            memcpy(data, content, (size_t)size);
        }
        void update(const void* content) {
            memcpy(data, content, (size_t)size);
        }
    private:
        void* data;
    };

    template<typename T>
    struct VectorBuffer : Buffer, Command {
        VectorBuffer() requires (std::is_same_v<T, Vertex> or std::is_same_v<T, glm::vec3>) {
            usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        VectorBuffer() requires (std::is_same_v<T, uint16_t>) {
            usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        ~VectorBuffer() {
            vkUnmapMemory(GPU::device, mStageBuffer.memory);
            destroyBuffer();
            freeMemory();
        }
    public:
        void init(std::vector<T> const& vector) {
            size = vector.size() * sizeof(T);
            properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            std::thread tStage([&]
                {// Prepare staging buffer
                    mStageBuffer.init(size);
                    mStageBuffer.stageData(vector.data());
                });
            std::thread tMain([&]
                {// Create VBO
                    createBuffer();
                    allocateMemory();
                });
            tMain.join(); tStage.join();

            copyBuffer();
        }
    protected:
        void bind(VkCommandBuffer& commandBuffer) requires (std::is_same_v<T, Vertex> or std::is_same_v<T, glm::vec3>) {
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &self, offsets);
        }
        void bind(VkCommandBuffer& commandBuffer) requires (std::is_same_v<T, uint16_t>) {
            vkCmdBindIndexBuffer(commandBuffer, self, 0, VK_INDEX_TYPE_UINT16);
        }
        void update(std::vector<T> const& vector) {
            mStageBuffer.update(vector.data());
            copyBuffer();
        }
    protected:
        void copyBuffer() {
            beginCommand();

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(Command::buffer, mStageBuffer.self, self, 1, &copyRegion);

            endCommand();
        }
    private:
        StageBuffer mStageBuffer;
        void destroyBuffer() {
            std::jthread tMain(vkDestroyBuffer, GPU::device, self, nullptr);
            std::jthread tStage(vkDestroyBuffer, GPU::device, mStageBuffer.self, nullptr);
        }
        void freeMemory() {
            std::jthread tMain(vkFreeMemory, GPU::device, memory, nullptr);
            std::jthread tStage(vkFreeMemory, GPU::device, mStageBuffer.memory, nullptr);
        }
    };

    
}

#endif