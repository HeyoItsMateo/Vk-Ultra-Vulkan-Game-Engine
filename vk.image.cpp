#include "vk.image.h"

namespace vk {
    void Image::createImage(GPU* const pGPU, vk::Image& image, uint32_t mipLevels)
    {
        VkImageCreateInfo imageInfo
        { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = image.format;
        imageInfo.tiling = image.tiling;
        imageInfo.usage = image.usage;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        imageInfo.extent = { pGPU->extent.width, pGPU->extent.height, 1 };
        imageInfo.samples = pGPU->msaaSamples;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;

        VK_CHECK_RESULT(vkCreateImage(pGPU->device, &imageInfo, nullptr, &image.image));

        mallocImage(pGPU, image);
    }

    void Image::createImageView(const VkDevice device, vk::Image& image, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo
        { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = image.image;
        viewInfo.format = image.format;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange = { image.aspect, 0, mipLevels, 0, 1 };

        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &image.view));
    }

    void Image::mallocImage(GPU* const pGPU, vk::Image& vkImage)
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(pGPU->device, vkImage.image, &memRequirements);

        VkMemoryAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GPU::findMemoryType(pGPU->physicalDevice, memRequirements.memoryTypeBits, vkImage.properties);

        VK_CHECK_RESULT(vkAllocateMemory(pGPU->device, &allocInfo, nullptr, &vkImage.memory));

        vkBindImageMemory(pGPU->device, vkImage.image, vkImage.memory, 0);
    }

    VkImageMemoryBarrier Image::createMemoryBarrier(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        VkImageMemoryBarrier memoryBarrier
        { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        memoryBarrier.image = image;
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.oldLayout = oldLayout;
        memoryBarrier.newLayout = newLayout;

        memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1 };

        std::array<VkAccessFlags, 2> accessFlags = transitionMap[{oldLayout, newLayout}];
        memoryBarrier.srcAccessMask = accessFlags[0];
        memoryBarrier.dstAccessMask = accessFlags[1];
        return memoryBarrier;
    }

    VkBufferImageCopy Image::createCopyRegion(VkExtent2D& extent, VkImageAspectFlags aspectFlags) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { extent.width, extent.height, 1 };
        region.imageSubresource = { aspectFlags, 0, 0, 1 };
        return region;
    }

    void Image::updateMemoryBarrier(VkImageMemoryBarrier& memoryBarrier, VkImageLayout oldLayout, VkImageLayout newLayout) {
        memoryBarrier.oldLayout = oldLayout;
        memoryBarrier.newLayout = newLayout;

        std::array<VkAccessFlags, 2> accessFlags = transitionMap[{oldLayout, newLayout}];
        memoryBarrier.srcAccessMask = accessFlags[0];
        memoryBarrier.dstAccessMask = accessFlags[1];
    }

    VkImageBlit Image::createBlit(int32_t mipWidth, int32_t mipHeight, uint32_t mipLevel) {
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mipLevel - 1, 0, 1 };

        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 0, 1 };
        return blit;
    }
}
