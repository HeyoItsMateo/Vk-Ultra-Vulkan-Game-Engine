#ifndef hBuffers
#define hBuffers

#include "vk.cpu.h"
#include <utility>

namespace vk {
    inline static void createBuffer(VkBuffer& buffer, VkDeviceSize& size, VkBufferUsageFlags usage);
    inline static void allocateMemory(VkBuffer& buffer, VkDeviceMemory& memory, VkMemoryPropertyFlags properties);

    /* Multi-Buffer */
    struct Buffer_ {
        Buffer_(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~Buffer_();
    public:
        VkDeviceSize size;
        std::vector<VkBuffer> buffer;
        std::vector<VkDeviceMemory> memory;
    };

    /* Solo-Buffer */
    struct Buffer {
        Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~Buffer();
    public:
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDeviceSize size;
    };

    /* Parallelized Staging Buffer (?) */
    struct StageBuffer_ : CPU_<> {
        StageBuffer_(const void* content, VkDeviceSize size);
        ~StageBuffer_();
    public:
        VkBuffer buffer;
        VkDeviceSize size;
        void update(const void* content, std::vector<VkBuffer>& dstBuffers);
        void transferData(std::vector<VkBuffer>& dstBuffers);
        void transferImage(VkImage& dstImage, VkExtent3D imageExtent);
    protected:
        void* data;
        VkDeviceMemory memory;
    };

    /* OG Staging Buffer*/
    struct StageBuffer : Command {
        StageBuffer(const void* content, VkDeviceSize size);
        ~StageBuffer();
    public:
        VkBuffer buffer;
        VkDeviceSize size;
        void update(const void* content, VkBuffer& dstBuffer);
        void transferData(VkBuffer& dstBuffer);
        void transferImage(VkImage& dstImage, VkExtent3D imageExtent);
    protected:
        void* data;
        VkDeviceMemory memory;
    };

    struct DataBuffer {
        DataBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~DataBuffer();
    public:
        std::vector<VkBuffer> buffers
        { MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE };
    protected:
        VkDeviceSize size;
        std::vector<VkDeviceMemory> _memory
        { MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE };
    };

    /* VBO-EBO Base */
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

            VK_CHECK_RESULT(vkCreateBuffer(GPU::device, &bufferInfo, nullptr, &buffer));

            allocateMemory(buffer, memory, properties);
        }
        void allocateMemory(VkBuffer& buffer, VkDeviceMemory& memory, VkMemoryPropertyFlags properties) {
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(GPU::device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, properties);

            VK_CHECK_RESULT(vkAllocateMemory(GPU::device, &allocInfo, nullptr, &memory));

            vkBindBufferMemory(GPU::device, buffer, memory, 0);
        }
    };
}

#endif