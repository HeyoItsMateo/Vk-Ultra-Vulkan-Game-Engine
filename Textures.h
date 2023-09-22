#ifndef hTextures
#define hTextures

#include "vk.cpu.h"
#include "vk.image.h"
#include "vk.buffers.h"
#include "descriptors.h"

namespace vk {
    struct Sampler : Descriptor {
        VkSampler sampler;
        Sampler(uint32_t mipLevels = 1, VkShaderStageFlagBits shaderFlags = VK_SHADER_STAGE_FRAGMENT_BIT) : Descriptor(VK_DESCRIPTOR_TYPE_SAMPLER, shaderFlags) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(GPU::physicalDevice, &properties);

            VkSamplerCreateInfo samplerInfo
            { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod = 0.f;
            samplerInfo.maxLod = mipLevels;
            samplerInfo.mipLodBias = 0.0f; // Optional

            if (vkCreateSampler(GPU::device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }

            writeDescriptorSets();
        }
        ~Sampler() {
            vkDestroySampler(GPU::device, sampler, nullptr);
        }
    private:
        void writeDescriptorSets(uint32_t bindingCount = 1) override {
            VkWriteDescriptorSet allocWrite
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            allocWrite.dstArrayElement = 0;
            allocWrite.descriptorCount = 1;
            allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount, allocWrite);

            VkDescriptorImageInfo allocImage{};
            allocImage.sampler = sampler;
            std::vector<VkDescriptorImageInfo> imageInfo(bindingCount, allocImage);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                descriptorWrites[i].dstSet = Sets[i];
                for (uint32_t j = 0; j < bindingCount; j++) {
                    descriptorWrites[j].dstBinding = j;
                    descriptorWrites[j].pImageInfo = &imageInfo[j];
                }
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };

    struct Texture : Image, Command, Descriptor {
        uint32_t mipLevels;
        VkExtent2D extent;
        int texChannels;
        Texture(const char* filename);
        ~Texture() = default;
    private:
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
            beginCommand();

            VkImageMemoryBarrier barrier
            { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = Image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                Command::cmdBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            endCommand();
        }
        void copyBufferToImage(VkBuffer& buffer) {
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
            region.imageExtent = {
                extent.width,
                extent.height,
                1
            };

            vkCmdCopyBufferToImage(Command::cmdBuffer, buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            endCommand();
        }
        void generateMipmaps() {
            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(GPU::physicalDevice, format, &formatProperties);

            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            beginCommand();

            VkImageMemoryBarrier barrier
            { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.image = Image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = extent.width;
            int32_t mipHeight = extent.height;

            for (uint32_t i = 1; i < mipLevels; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(Command::cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(Command::cmdBuffer,
                    Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(Command::cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(Command::cmdBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            endCommand();
        }

        void writeDescriptorSets(uint32_t bindingCount = 1) override {
            VkWriteDescriptorSet allocWrite
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            allocWrite.dstArrayElement = 0;
            allocWrite.descriptorCount = 1;
            allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount, allocWrite);

            VkDescriptorImageInfo allocImage{};
            allocImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            allocImage.imageView = ImageView;
            std::vector<VkDescriptorImageInfo> imageInfo(bindingCount, allocImage);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                descriptorWrites[i].dstSet = Sets[i];
                for (uint32_t j = 0; j < bindingCount; j++) {
                    descriptorWrites[j].dstBinding = j;
                    descriptorWrites[j].pImageInfo = &imageInfo[j];
                }
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };

    struct ComputeImage : Image, Command, Descriptor {
        ComputeImage(VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_VERTEX_BIT)
            : Descriptor(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT | stageFlags)
        {
            VkExtent2D extent = { 30, 20 };

            createImage(Image, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
            allocateImageMemory(Image, ImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            setImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
            
            createImageView(Image, ImageView);

            writeDescriptorSets();
        }
    private:
        void createImage(VkImage& image, VkExtent2D& extent, VkImageUsageFlags usageFlags, VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL) {
            VkImageCreateInfo imageInfo
            { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageInfo.usage = usageFlags;
            imageInfo.tiling = imgTiling;

            imageInfo.extent = { extent.width, extent.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            imageInfo.arrayLayers = 1;
            
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            //imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VK_CHECK_RESULT(vkCreateImage(GPU::device, &imageInfo, nullptr, &Image));
        }
        void allocateImageMemory(VkImage& image, VkDeviceMemory& imageMemory, VkMemoryPropertyFlags memProperties) {
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(GPU::device, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, memProperties);

            VK_CHECK_RESULT(vkAllocateMemory(GPU::device, &allocInfo, nullptr, &imageMemory));

            vkBindImageMemory(GPU::device, image, imageMemory, 0);
        }
        void createImageView(VkImage& image, VkImageView& imageView, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
            VkImageViewCreateInfo viewInfo
            { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            viewInfo.image = image;
            viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            /* Subresource Info */
            viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            VK_CHECK_RESULT(vkCreateImageView(GPU::device, &viewInfo, nullptr, &imageView));
        }

        void setImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
            
            beginCommand();
            VkImageMemoryBarrier memoryBarrier
            { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            memoryBarrier.image = Image;
            memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            memoryBarrier.oldLayout = oldLayout;
            memoryBarrier.newLayout = newLayout;
            
            memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            std::array<VkAccessFlags, 2> accessFlags = transitionMap[{oldLayout, newLayout}];
            memoryBarrier.srcAccessMask = accessFlags[0];
            memoryBarrier.dstAccessMask = accessFlags[1];

            vkCmdPipelineBarrier(
                Command::cmdBuffer,
                srcStageMask, dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &memoryBarrier
            );

            endCommand();
        }
        void writeDescriptorSets(uint32_t bindingCount = 1) override {
            VkWriteDescriptorSet allocWrite
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            allocWrite.dstArrayElement = 0;
            allocWrite.descriptorCount = 1;
            allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount, allocWrite);

            VkDescriptorImageInfo allocImage{};
            allocImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            allocImage.imageView = ImageView;
            std::vector<VkDescriptorImageInfo> imageInfo(bindingCount, allocImage);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                descriptorWrites[i].dstSet = Sets[i];
                for (uint32_t j = 0; j < bindingCount; j++) {
                    descriptorWrites[j].dstBinding = j;
                    descriptorWrites[j].pImageInfo = &imageInfo[j];
                }
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };

    struct CombinedImageSampler : Image, Command, Descriptor {
        uint32_t mipLevels;

        VkSampler Sampler;
        VkExtent2D extent;
        int texChannels;

        CombinedImageSampler(const char* filename);
        ~CombinedImageSampler() {
            vkDestroySampler(GPU::device, Sampler, nullptr);
        }
    private:
        void createImage(VkSampleCountFlagBits numSamples);
        void createImageView(VkFormat format, VkImageAspectFlags aspectFlags) {
            VkImageViewCreateInfo viewInfo
            { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            viewInfo.image = Image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(GPU::device, &viewInfo, nullptr, &ImageView) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image view!");
            }
        }

        void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
            beginCommand();

            VkImageMemoryBarrier barrier
            { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = Image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                Command::cmdBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            endCommand();
        }
        void copyBufferToImage(VkBuffer& buffer) {
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
            region.imageExtent = {
                extent.width,
                extent.height,
                1
            };

            vkCmdCopyBufferToImage(Command::cmdBuffer, buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            endCommand();
        }
        void generateMipmaps() {
            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(GPU::physicalDevice, format, &formatProperties);

            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            beginCommand();

            VkImageMemoryBarrier barrier
            { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.image = Image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = extent.width;
            int32_t mipHeight = extent.height;

            for (uint32_t i = 1; i < mipLevels; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(Command::cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(Command::cmdBuffer,
                    Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(Command::cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(Command::cmdBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            endCommand();
        }

        void createTextureSampler() {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(GPU::physicalDevice, &properties);

            VkSamplerCreateInfo samplerInfo
            { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod = 0.f;
            samplerInfo.maxLod = static_cast<float>(mipLevels);
            samplerInfo.mipLodBias = 0.0f; // Optional

            if (vkCreateSampler(GPU::device, &samplerInfo, nullptr, &Sampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }
        }

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
            VkBufferCreateInfo bufferInfo
            { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(GPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(GPU::device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(GPU::device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(GPU::device, buffer, bufferMemory, 0);
        }
        void writeDescriptorSets(uint32_t bindingCount = 1) override {
            VkWriteDescriptorSet allocWrite
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            allocWrite.dstArrayElement = 0;
            allocWrite.descriptorCount = 1;
            allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount, allocWrite);

            VkDescriptorImageInfo allocImage{};
            allocImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            allocImage.imageView = ImageView;
            allocImage.sampler = Sampler;
            std::vector<VkDescriptorImageInfo> imageInfo(bindingCount, allocImage);

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                descriptorWrites[i].dstSet = Sets[i];
                for (uint32_t j = 0; j < bindingCount; j++) {
                    descriptorWrites[j].dstBinding = j;
                    descriptorWrites[j].pImageInfo = &imageInfo[j];
                } 
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };
}

#endif