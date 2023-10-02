#include "vk.image.h"

namespace vk {
    Image::~Image()
    {
        std::jthread t0(vkDestroyImageView, GPU::device, ImageView, nullptr);
        std::jthread t1(vkDestroyImage, GPU::device, Image, nullptr);
        vkFreeMemory(GPU::device, ImageMemory, nullptr);
    }
    VkFormat Image::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(GPU::physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
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

    void Image::createImage(vk::Image& image, VkSampleCountFlagBits msaaCount, uint32_t mipLevels) {
        VkImageCreateInfo imageInfo
        { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = image.format;
        imageInfo.tiling = image.tiling;
        imageInfo.usage = image.usage;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        imageInfo.extent = { image.extent.width, image.extent.height, 1 };
        imageInfo.samples = msaaCount;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;

        VK_CHECK_RESULT(vkCreateImage(GPU::device, &imageInfo, nullptr, &image.Image));

        allocateMemory(image);
    }
    void Image::allocateMemory(vk::Image& image) {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(GPU::device, image.Image, &memRequirements);

        VkMemoryAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, image.properties);

        VK_CHECK_RESULT(vkAllocateMemory(GPU::device, &allocInfo, nullptr, &image.ImageMemory));

        vkBindImageMemory(GPU::device, image.Image, image.ImageMemory, 0);
    }
    void Image::createImageView(vk::Image& image, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo
        { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = image.Image;
        viewInfo.format = image.format;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange = { image.aspect, 0, mipLevels, 0, 1 };

        VK_CHECK_RESULT(vkCreateImageView(GPU::device, &viewInfo, nullptr, &image.ImageView));
    }


}
