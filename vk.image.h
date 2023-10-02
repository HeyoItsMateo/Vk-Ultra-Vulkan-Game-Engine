#pragma once

#ifndef hImage
#define hImage

#include "vk.gpu.h"

#include <thread>
#include <execution>
#include <map>

inline std::map<std::array<VkImageLayout, 2>, std::array<VkAccessFlags, 2>> transitionMap{
    {
        {VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL},
        { VK_ACCESS_NONE, VK_ACCESS_NONE } },
    {
        {VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL},
        {VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT} },
    {
        {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
        {VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT} },
    {
        {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT} },
    {
        {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT} }
};

namespace vk {
    struct Image {
        VkImage Image;
        VkImageView ImageView;
        VkDeviceMemory ImageMemory;
        ~Image();
    protected:
        static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        
        static VkImageMemoryBarrier createMemoryBarrier(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);
        static VkBufferImageCopy createCopyRegion(VkExtent2D& extent, VkImageAspectFlags aspectFlags);
        static void updateMemoryBarrier(VkImageMemoryBarrier& memoryBarrier, VkImageLayout oldLayout, VkImageLayout newLayout);
        static VkImageBlit createBlit(int32_t mipWidth, int32_t mipHeight, uint32_t mipLevel);
    public:
        VkExtent2D extent;
        uint32_t mipLevels = 1;

        VkFormat format;
        VkImageUsageFlags usage;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        static void createImage(vk::Image& image, VkSampleCountFlagBits msaaCount = GPU::msaaSamples, uint32_t mipLevels = 1);
        static void allocateMemory(vk::Image& image);
        
        static void createImageView(vk::Image& image, uint32_t mipLevels = 1);

        void createResource() {
            extent = GPU::Extent;
            createImage(*this);
            createImageView(*this);
        }

        void destroyResource() {
            vkDestroyImageView(GPU::device, ImageView, nullptr);
            vkDestroyImage(GPU::device, Image, nullptr);
            vkFreeMemory(GPU::device, ImageMemory, nullptr);
        }
    };
    /*------------------------------------------*/
    struct Color : Image {
        Color() {
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
    /*------------------------------------------*/
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