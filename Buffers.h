#ifndef hBuffers
#define hBuffers

namespace vk {
    struct Buffer {
        Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        {
            this->size = size;
            createBuffer(size, usage);
            allocateMemory(properties);
        }
        ~Buffer() {
            vkDestroyBuffer(GPU::device, buffer, nullptr);
            vkFreeMemory(GPU::device, memory, nullptr);
        }
    public:
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDeviceSize size;
    private:
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage) {
            VkBufferCreateInfo bufferInfo
            { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(GPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }
        }
        void allocateMemory(VkMemoryPropertyFlags properties) {
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
    };

    struct StageBuffer : Command {
        StageBuffer(const void* content, VkDeviceSize size) : size(size)
        {
            createBuffer();
            allocateMemory();
            stageData(content);
        }
        ~StageBuffer() {
            vkUnmapMemory(GPU::device, memory);
            vkDestroyBuffer(GPU::device, buffer, nullptr);
            vkFreeMemory(GPU::device, memory, nullptr);
        }
    public:
        VkBuffer buffer;
        void update(const void* content, VkBuffer& dstBuffer) {
            memcpy(data, content, static_cast<size_t>(size));
            transferData(dstBuffer);
        }
        void transferData(VkBuffer& dstBuffer) {
            beginCommand();

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(Command::cmdBuffer, buffer, dstBuffer, 1, &copyRegion);

            endCommand();
        }
        void transferImage(VkImage& dstImage, VkExtent3D imageExtent) {
            beginCommand();

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = imageExtent;

            vkCmdCopyBufferToImage(Command::cmdBuffer, buffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            endCommand();
        }
    protected:
        void* data;
        VkDeviceMemory memory;
        VkDeviceSize size;
    private:
        void createBuffer() {
            VkBufferCreateInfo bufferInfo
            { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(GPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }
        }
        void allocateMemory() {
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(GPU::device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            if (vkAllocateMemory(GPU::device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(GPU::device, buffer, memory, 0);
        }

        void stageData(const void* content) {
            vkMapMemory(GPU::device, memory, 0, size, 0, &data);
            memcpy(data, content, (size_t)size);
        }    
    };

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

            allocateMemory(buffer, memory, properties);
        }
        void allocateMemory(VkBuffer& buffer, VkDeviceMemory& memory, VkMemoryPropertyFlags properties) {
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
            vkCmdCopyBuffer(Command::cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            endCommand();
        }
    };
}
#endif