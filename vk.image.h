#ifndef hImage
#define hImage

#include "vk.gpu.h"

#include <thread>
#include <execution>


namespace vk {
    struct Image {
        VkImage Image;
        VkImageView ImageView;
        VkDeviceMemory ImageMemory;
        ~Image() {
            std::jthread t0(vkDestroyImageView, GPU::device, ImageView, nullptr);
            std::jthread t1(vkDestroyImage, GPU::device, Image, nullptr);
            vkFreeMemory(GPU::device, ImageMemory, nullptr);
        }
    protected:
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
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

        static VkImageMemoryBarrier createImageMemoryBarrier(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);
    public:
        VkImageAspectFlagBits aspect{};
        VkFormat format;
        VkImageUsageFlags usage;
        
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        void createImage(VkExtent2D& imageExtent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits msaaCount = GPU::msaaSamples, uint32_t mipLevels = 1) {
            VkImageCreateInfo imageInfo
            { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = imageExtent.width;
            imageInfo.extent.height = imageExtent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = msaaCount;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VK_CHECK_RESULT(vkCreateImage(GPU::device, &imageInfo, nullptr, &Image));

            allocateMemory(Image, ImageMemory, properties);
        }
        static void createImageView(VkImage& image, VkImageView& view, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
            VkImageViewCreateInfo viewInfo
            { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            viewInfo.image = image;
            viewInfo.format = format;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VK_CHECK_RESULT(vkCreateImageView(GPU::device, &viewInfo, nullptr, &view));
        }
        
        static void allocateMemory(VkImage& image, VkDeviceMemory& imageMemory, VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(GPU::device, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, properties);

            VK_CHECK_RESULT(vkAllocateMemory(GPU::device, &allocInfo, nullptr, &imageMemory));

            vkBindImageMemory(GPU::device, image, imageMemory, 0);
        }

        void createResource() {
            createImage(GPU::Extent, format, tiling, usage, properties);
            createImageView(Image, ImageView, format, aspect, 1);
        }

        void destroyResource() {
            vkDestroyImageView(GPU::device, ImageView, nullptr);
            vkDestroyImage(GPU::device, Image, nullptr);
            vkFreeMemory(GPU::device, ImageMemory, nullptr);
        }
    };

    struct Color : Image {
        Color() {
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            format = VK_FORMAT_B8G8R8A8_SRGB;
            usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        static VkAttachmentDescription createAttachment() {
            VkAttachmentDescription attachment{};
            attachment.format = VK_FORMAT_B8G8R8A8_SRGB;
            attachment.samples = GPU::msaaSamples;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            return attachment;
        }
        static VkAttachmentDescription createResolve() {
            VkAttachmentDescription resolve{};
            resolve.format = VK_FORMAT_B8G8R8A8_SRGB;
            resolve.samples = VK_SAMPLE_COUNT_1_BIT;
            resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            return resolve;
        }
    };
    struct Depth : Image {
        Depth() {   
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            format = findSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
            usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        VkAttachmentDescription createAttachment() {
            VkAttachmentDescription attachment{};
            attachment.format = format;
            attachment.samples = GPU::msaaSamples;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            return attachment;
        }
    };
}
#endif