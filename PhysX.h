#ifndef hPhysX
#define hPhysX

namespace vk {
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
}


#endif