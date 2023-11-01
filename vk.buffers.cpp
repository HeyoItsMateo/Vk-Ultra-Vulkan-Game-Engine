#include "vk.buffers.h"
namespace vk {
    void createBuffer(VkBuffer& buffer, VkDeviceSize& size, VkBufferUsageFlags usage) {
        VkBufferCreateInfo bufferInfo
        { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateBuffer(GPU::device, &bufferInfo, nullptr, &buffer));
    }
    void allocateMemory(VkBuffer& buffer, VkDeviceMemory& memory, VkMemoryPropertyFlags properties) {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(GPU::device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, properties);

        VK_CHECK_RESULT(vkAllocateMemory(GPU::device, &allocInfo, nullptr, &memory));

        VK_CHECK_RESULT(vkBindBufferMemory(GPU::device, buffer, memory, 0));
    }

    /* Multi-Buffer */
    Buffer_::Buffer_(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        this->size = size;
        buffer.resize(MAX_FRAMES_IN_FLIGHT);
        memory.resize(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(buffer[i], size, usage);
            allocateMemory(buffer[i], memory[i], properties);
        }
    }
    Buffer_::~Buffer_() {
        std::for_each(std::execution::par, buffer.begin(), buffer.end(),
            [&](VkBuffer buffy) { vkDestroyBuffer(GPU::device, buffy, nullptr); });

        std::for_each(std::execution::par, memory.begin(), memory.end(),
            [&](VkDeviceMemory memy) { vkFreeMemory(GPU::device, memy, nullptr); });
    }

    /* Solo-Buffer */
    Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        this->size = size;
        createBuffer(buffer, size, usage);
        allocateMemory(buffer, memory, properties);
    }
    Buffer::~Buffer() {
        vkDestroyBuffer(GPU::device, buffer, nullptr);
        vkFreeMemory(GPU::device, memory, nullptr);
    }
    
    /* Staging Buffer */
    StageBuffer_::StageBuffer_(const void* content, VkDeviceSize size) : size(size)
    {
        createBuffer(buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        allocateMemory(buffer, memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(GPU::device, memory, 0, size, 0, &data);
        memcpy(data, content, (size_t)size);
    }
    StageBuffer_::~StageBuffer_() {
        vkUnmapMemory(GPU::device, memory);
        vkDestroyBuffer(GPU::device, buffer, nullptr);
        vkFreeMemory(GPU::device, memory, nullptr);
    }
    /* Public */
    void StageBuffer_::update(const void* content, std::vector<VkBuffer>& dstBuffers) {
        memcpy(data, content, static_cast<size_t>(size));
        transferData(dstBuffers);
    }

    void StageBuffer_::transferData(std::vector<VkBuffer>& dstBuffers) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkBeginCommandBuffer(cmdBuffers[i], &beginInfo);

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(cmdBuffers[i], buffer, dstBuffers[i], 1, &copyRegion);

            vkEndCommandBuffer(cmdBuffers[i]);
        }
        submitCommands(cmdBuffers, MAX_FRAMES_IN_FLIGHT);
    }
    void StageBuffer_::transferImage(VkImage& dstImage, VkExtent3D imageExtent) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkBeginCommandBuffer(cmdBuffers[i], &beginInfo);

            VkBufferImageCopy region{};
            region.imageExtent = imageExtent;
            region.imageOffset = { 0, 0, 0 };
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

            vkCmdCopyBufferToImage(cmdBuffers[i], buffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            vkEndCommandBuffer(cmdBuffers[i]);
        }
        submitCommands(cmdBuffers, MAX_FRAMES_IN_FLIGHT);
    }

    /* OG Staging Buffer */
    StageBuffer::StageBuffer(const void* content, VkDeviceSize size) : size(size) {
        createBuffer(buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        allocateMemory(buffer, memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(GPU::device, memory, 0, size, 0, &data);
        memcpy(data, content, (size_t)size);
    }
    StageBuffer::~StageBuffer() {
        if (memory != VK_NULL_HANDLE) {
            vkUnmapMemory(GPU::device, memory);
            vkDestroyBuffer(GPU::device, buffer, nullptr);
            vkFreeMemory(GPU::device, memory, nullptr);
        }
    }
    /* Public */
    void StageBuffer::update(const void* content, VkBuffer& dstBuffer) {
        memcpy(data, content, static_cast<size_t>(size));
        transferData(dstBuffer);
    }
    void StageBuffer::transferData(VkBuffer& dstBuffer) {
        beginCommand();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(Command::cmdBuffer, buffer, dstBuffer, 1, &copyRegion);

        endCommand();
    }
    void StageBuffer::transferImage(VkImage& dstImage, VkExtent3D imageExtent) {
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

    /* UBO-SSBO Base */
    DataBuffer::DataBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) : size(size) {
        std::array<int, MAX_FRAMES_IN_FLIGHT> idx{};
        std::iota(idx.begin(), idx.end(), 0);
        std::for_each(idx.begin(), idx.end(), [&](int i) {
            createBuffer(buffers[i], size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage);
            allocateMemory(buffers[i], _memory[i], properties);
            });
    }
    DataBuffer::~DataBuffer() {
        std::for_each(std::execution::par, buffers.begin(), buffers.end(),
            [&](VkBuffer buffer) { vkDestroyBuffer(GPU::device, buffer, nullptr); });

        std::for_each(std::execution::par, _memory.begin(), _memory.end(),
            [&](VkDeviceMemory memory) { vkFreeMemory(GPU::device, memory, nullptr); });
    }

}

