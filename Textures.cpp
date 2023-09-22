#include "Textures.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vk {
    Texture::Texture(const char* filename) : Descriptor(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        format = VK_FORMAT_R8G8B8A8_SRGB;
        usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        stbi_uc* pixels = stbi_load(filename, (int*)&extent.width, (int*)&extent.height, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
        createImage(extent, VK_SAMPLE_COUNT_1_BIT, mipLevels);

        VkDeviceSize imageSize = extent.width * extent.height * 4;
        StageBuffer stagePixels(pixels, imageSize);
        stbi_image_free(pixels);

        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagePixels.buffer);
        generateMipmaps();

        createImageView(Image, ImageView, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        writeDescriptorSets();
    }

    
    CombinedImageSampler::CombinedImageSampler(const char* filename)
        : Descriptor(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        format = VK_FORMAT_R8G8B8A8_SRGB;
        usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        stbi_uc* pixels = stbi_load(filename, (int*)&extent.width, (int*)&extent.height, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
        createImage(VK_SAMPLE_COUNT_1_BIT);

        VkDeviceSize imageSize = extent.width * extent.height * 4;
        StageBuffer stagePixels(pixels, imageSize);
        stbi_image_free(pixels);

        transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagePixels.buffer);
        //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

        generateMipmaps();
        createImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        createTextureSampler();

        writeDescriptorSets();
    }


    void CombinedImageSampler::createImage(VkSampleCountFlagBits numSamples)
    {
        VkImageCreateInfo imageInfo
        { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.usage = usage;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.extent = { extent.width, extent.height, 1 };

        imageInfo.samples = numSamples;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;

        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateImage(GPU::device, &imageInfo, nullptr, &Image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(GPU::device, Image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GPU::findMemoryType(memRequirements.memoryTypeBits, properties);

        VK_CHECK_RESULT(vkAllocateMemory(GPU::device, &allocInfo, nullptr, &ImageMemory));
        vkBindImageMemory(GPU::device, Image, ImageMemory, 0);
    }
}