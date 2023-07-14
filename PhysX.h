#ifndef hPhysX
#define hPhysX

struct PhxBuffer {
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

        if (vkCreateBuffer(VkGPU::device, &bufferInfo, nullptr, &self) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }
    }
    void allocateMemory() {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(VkGPU::device, self, &memRequirements);

        VkMemoryAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VkGPU::findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(VkGPU::device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(VkGPU::device, self, memory, 0);
    }
};

#endif