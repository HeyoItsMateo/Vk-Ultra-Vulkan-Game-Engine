#pragma once
#ifndef hTextures
#define hTextures

#include "vk.cpu.h"
#include "vk.image.h"
#include "vk.buffers.h"
#include "descriptors.h"

namespace vk {
    struct Sampler : Descriptor {
        VkSampler sampler;
        Sampler(uint32_t mipLevels = 1, VkShaderStageFlagBits shaderFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
            : Descriptor(VK_DESCRIPTOR_TYPE_SAMPLER, shaderFlags)
        {
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

            VK_CHECK_RESULT(vkCreateSampler(GPU::device, &samplerInfo, nullptr, &sampler));

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

            for (uint32_t i = 0; i < bindingCount; i++) {
                descriptorWrites[i].dstSet = Sets[i];
                descriptorWrites[i].dstBinding = i;
                descriptorWrites[i].pImageInfo = &imageInfo[i];
            }
            vkUpdateDescriptorSets(GPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    };
    /*------------------------------------------*/
    struct Texture : Image, Command, Descriptor {
        int texChannels;
        Texture(const char* filename);
        ~Texture() = default;
    private:
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer& buffer);
        void generateMipmaps();

        void writeDescriptorSets(uint32_t bindingCount = 1) override;
    };
    /*------------------------------------------*/
    struct CombinedImageSampler : Image, Command, Descriptor {
        VkSampler Sampler;
        int texChannels;

        CombinedImageSampler(const char* filename);
        ~CombinedImageSampler();
    private:
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer& buffer);
        void generateMipmaps();

        void createTextureSampler();

        void writeDescriptorSets(uint32_t bindingCount = 1) override;
    };
    /*------------------------------------------*/
    struct ComputeImage : Image, CPU_<1>, Descriptor {
        ComputeImage(VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_VERTEX_BIT)
            : Descriptor(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT | stageFlags)
        {
            format = VK_FORMAT_R8G8B8A8_UNORM;
            usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            extent = { 3000, 2000 };

            createImage(*this, VK_SAMPLE_COUNT_1_BIT);

            setImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

            createImageView(*this);

            writeDescriptorSets();
        }
    private:
        void setImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
            VkImageMemoryBarrier memoryBarrier = createMemoryBarrier(Image, oldLayout, newLayout);

            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            beginCommand(*cmdBuffers);
            vkCmdPipelineBarrier(
                *cmdBuffers,
                srcStageMask, dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &memoryBarrier
            );
            endCommand(*cmdBuffers);

            submitCommands(cmdBuffers, 1);
        }
        void writeDescriptorSets(uint32_t bindingCount = 1) override {
            VkWriteDescriptorSet allocWrite
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            allocWrite.dstArrayElement = 0;
            allocWrite.descriptorCount = 1;
            allocWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            std::vector<VkWriteDescriptorSet> descriptorWrites(MAX_FRAMES_IN_FLIGHT, allocWrite);

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
}

#endif