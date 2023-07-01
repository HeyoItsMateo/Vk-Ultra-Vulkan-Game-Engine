#include "VulkanGPU.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <random>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;

    static VkVertexInputBindingDescription vkCreateBindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> vkCreateAttributes() {
        std::vector<VkVertexInputAttributeDescription> Attributes{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Particle, position)},
            {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, color)}
        };
        return Attributes;
    }
};

struct camMatrix {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct UBO {
    camMatrix camera;
    float dt;
    void update(int width, int height) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        camera.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        camera.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        camera.proj = glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 10.0f);
        camera.proj[1][1] *= -1;
    }
};

struct SSBO {
    std::vector<Particle> particles{ 1000 };
    SSBO(int width, int height) {
        populate(particles, width, height);
    }
private:
    void populate(std::vector<Particle>& particles, int width, int height) {
        std::default_random_engine rndEngine((unsigned)time(nullptr));
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

        for (auto& particle : particles) {
            float r = 0.25f * sqrt(rndDist(rndEngine));
            float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
            float x = r * cos(theta) * height / width;
            float y = r * sin(theta);
            float z = r * cos(theta) / sin(theta);
            particle.position = glm::vec3(x, y, z);
            particle.velocity = glm::normalize(glm::vec3(x, y, z)) * 0.00025f;
            particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
        }

    }
};

struct memBuffer {
    VkGPU* pGPU;
    void createBuffer(VkGPU& GPU, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(GPU.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(GPU.device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GPU.findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(GPU.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(GPU.device, buffer, bufferMemory, 0);
    }
};

struct VkTexture : memBuffer {
    VkImage Image;
    VkImageView ImageView;
    VkDeviceMemory ImageMemory;
    uint32_t mipLevels;

    VkSampler Sampler;
    int texWidth, texHeight, texChannels;

    VkCPU CPU;

    VkTexture(VkGPU& GPU, const char* filename) : CPU(GPU) {
        pGPU = &GPU;

        stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(GPU, imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(GPU.device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(GPU.device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer);
        //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

        vkDestroyBuffer(GPU.device, stagingBuffer, nullptr);
        vkFreeMemory(GPU.device, stagingBufferMemory, nullptr);

        generateMipmaps(VK_FORMAT_R8G8B8A8_SRGB);
        createImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        createTextureSampler();
    }
    ~VkTexture() {
        vkDestroySampler(pGPU->device, Sampler, nullptr);
        vkDestroyImageView(pGPU->device, ImageView, nullptr);

        vkDestroyImage(pGPU->device, Image, nullptr);
        vkFreeMemory(pGPU->device, ImageMemory, nullptr);
    }
private:
    void createImage(VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(pGPU->device, &imageInfo, nullptr, &Image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(pGPU->device, Image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = pGPU->findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(pGPU->device, &allocInfo, nullptr, &ImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(pGPU->device, Image, ImageMemory, 0);
    }
    void createImageView(VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(pGPU->device, &viewInfo, nullptr, &ImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }
    }

    void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = CPU.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        CPU.endSingleTimeCommands(commandBuffer);
    }
    void copyBufferToImage(VkBuffer buffer) {
        VkCommandBuffer commandBuffer = CPU.beginSingleTimeCommands();

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
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight),
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        CPU.endSingleTimeCommands(commandBuffer);
    }
    void generateMipmaps(VkFormat imageFormat) {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(pGPU->physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = CPU.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = Image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
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

            vkCmdBlitImage(commandBuffer,
                Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
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

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        CPU.endSingleTimeCommands(commandBuffer);

    }

    void createTextureSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(pGPU->physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
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

        if (vkCreateSampler(pGPU->device, &samplerInfo, nullptr, &Sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }
};

template<typename T>
struct VkBufferObject : memBuffer {
    VkCPU CPU;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkBufferObject(VkGraphicsUnit& VkGPU, std::vector<T> const& content) : CPU(VkGPU) {
        this->pVkGPU = &VkGPU;
        VkDeviceSize bufferSize = sizeof(T) * content.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(VkGPU, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(VkGPU.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, content.data(), (size_t)bufferSize);
        vkUnmapMemory(VkGPU.device, stagingBufferMemory);

        createBuffer(VkGPU, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer, memory);

        copyBuffer(stagingBuffer, buffer, bufferSize);

        vkDestroyBuffer(VkGPU.device, stagingBuffer, nullptr);
        vkFreeMemory(VkGPU.device, stagingBufferMemory, nullptr);
    }
    ~VkBufferObject() {
        vkDestroyBuffer(pVkGPU->device, buffer, nullptr);
        vkFreeMemory(pVkGPU->device, memory, nullptr);
    }
private:
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = CPU.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        CPU.endSingleTimeCommands(commandBuffer);
    }
};

template<typename T>
struct VkDataBuffer : memBuffer {
    VkCPU CPU;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    std::vector<VkBuffer> buffer;
    std::vector<VkDeviceMemory> memory;
    VkDeviceSize bufferSize;

    VkDataBuffer(VkGraphicsUnit& VkGPU, T& ubo) : CPU(VkGPU) {
        this->pVkGPU = &VkGPU;
        buffer.resize(MAX_FRAMES_IN_FLIGHT);
        memory.resize(MAX_FRAMES_IN_FLIGHT);

        bufferSize = sizeof(T);
        createBuffer(VkGPU, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(VkGPU.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, &ubo, (size_t)bufferSize);
        vkUnmapMemory(VkGPU.device, stagingBufferMemory);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(VkGPU, bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                buffer[i], memory[i]);

            copyBuffer(stagingBuffer, buffer[i], bufferSize);
            //vkMapMemory(pVkGPU->device, memory[i], 0, bufferSize, 0, &map[i]);
        }
    }
    void update(T& ubo, uint32_t currentImage, int width, int height) {
        ubo.update(width, height);

        void* data;
        vkMapMemory(pVkGPU->device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, &ubo.camera, (size_t)bufferSize);
        vkUnmapMemory(pVkGPU->device, stagingBufferMemory);

        copyBuffer(stagingBuffer, buffer[currentImage], bufferSize);
        //memcpy(buffer[currentImage], &ubo, sizeof(ubo));
    }
    ~VkDataBuffer() {
        vkDestroyBuffer(pVkGPU->device, stagingBuffer, nullptr);
        vkFreeMemory(pVkGPU->device, stagingBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(pVkGPU->device, buffer[i], nullptr);
            vkFreeMemory(pVkGPU->device, memory[i], nullptr);
        }
    }
private:
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = CPU.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        CPU.endSingleTimeCommands(commandBuffer);
    }
};

struct VkDescriptor {
    VkDescriptorPool Pool;
    VkDescriptorSetLayout SetLayout;
    std::vector<VkDescriptorSet> Sets;
};

struct VkUniformSet : VkDescriptor {
    VkUniformSet(VkGPU& GPU, VkDataBuffer<UBO>& data, VkTexture& texture, std::vector<VkDescriptorType>& type, VkShaderStageFlags flags) {
        pGPU = &GPU;
        createDescriptorSetLayout(type, &flags);
        createDescriptorPool();
        createDescriptorSets(data, texture);
    }
    ~VkUniformSet() {
        vkDestroyDescriptorPool(pGPU->device, Pool, nullptr);
        vkDestroyDescriptorSetLayout(pGPU->device, SetLayout, nullptr);
    }

private:
    VkGPU* pGPU;
    VkDescriptorType Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    void createDescriptorSetLayout(std::vector<VkDescriptorType>& type, VkShaderStageFlags flags[]) {

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (size_t i = 0; i < type.size(); i++) {
            bindings.push_back(VkUtils::bindSetLayout(i, type[i], flags[i]));
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(pGPU->device, &layoutInfo, nullptr, &SetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create uniform set layout!");
        }
    }
    void createDescriptorPool() {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (size_t i = 0; i < 1; i++) {
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) });
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) });
        }
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(pGPU->device, &poolInfo, nullptr, &Pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create uniform pool!");
        }
    }
    void createDescriptorSets(VkDataBuffer<UBO>& data, VkTexture& texture) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, SetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(pGPU->device, &allocInfo, Sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate uniform sets!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites(2);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = data.buffer[i];
            bufferInfo.offset = 0;
            bufferInfo.range = data.bufferSize;

            VkWriteDescriptorSet writeUBO = VkUtils::writeDescriptor(0, Type, Sets[i]);
            writeUBO.pBufferInfo = &bufferInfo;
            descriptorWrites[0] = writeUBO;

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.ImageView; // TODO: Generalize
            imageInfo.sampler = texture.Sampler; // TODO: Generalize

            VkWriteDescriptorSet writeSampler = VkUtils::writeDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Sets[i]);
            writeSampler.pImageInfo = &imageInfo;
            descriptorWrites[1] = writeSampler;

            vkUpdateDescriptorSets(pGPU->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
};

struct VkStorageSet : VkDescriptor {

    VkStorageSet(VkGPU& GPU, VkDataBuffer<SSBO>& data, VkShaderStageFlags flags) {
        pGPU = &GPU;
        createDescriptorSetLayout(&flags);
        createDescriptorPool();
        createDescriptorSets(data);
    }
    ~VkStorageSet() {
        vkDestroyDescriptorPool(pGPU->device, Pool, nullptr);
        vkDestroyDescriptorSetLayout(pGPU->device, SetLayout, nullptr);
    }

private:
    VkGPU* pGPU;
    VkDescriptorType Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    void createDescriptorSetLayout(VkShaderStageFlags flags[]) {

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (size_t i = 0; i < sizeof(*flags)/sizeof(flags[0]); i++) {
            bindings.push_back(VkUtils::bindSetLayout(i, Type, flags[i]));
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(pGPU->device, &layoutInfo, nullptr, &SetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader storage set layout!");
        }
    }
    void createDescriptorPool() {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (size_t i = 0; i < 1; i++) {
            poolSizes.push_back({ Type, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) });
        }
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(pGPU->device, &poolInfo, nullptr, &Pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader storage pool!");
        }
    }
    void createDescriptorSets(VkDataBuffer<SSBO>& data) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, SetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(pGPU->device, &allocInfo, Sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate shader storage sets!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites(1);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = data.buffer[i];
            bufferInfo.offset = 0;
            bufferInfo.range = data.bufferSize;

            VkWriteDescriptorSet writeUBO = VkUtils::writeDescriptor(0, Type, Sets[i]);
            writeUBO.pBufferInfo = &bufferInfo;
            descriptorWrites[0] = writeUBO;

            vkUpdateDescriptorSets(pGPU->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
};

struct VkTextureSet : VkDescriptor {
    VkTextureSet(VkGPU& GPU, VkTexture& texture, VkShaderStageFlags flags) {
        pGPU = &GPU;
        createDescriptorSetLayout(&flags);
        createDescriptorPool();
        createDescriptorSets(texture);
    }
    ~VkTextureSet() {
        vkDestroyDescriptorPool(pGPU->device, Pool, nullptr);
        vkDestroyDescriptorSetLayout(pGPU->device, SetLayout, nullptr);
    }
private:
    VkGPU* pGPU;
    VkDescriptorType Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    void createDescriptorSetLayout(VkShaderStageFlags flags[]) {

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (size_t i = 0; i < sizeof(*flags)/sizeof(flags[0]); i++) {
            bindings.push_back(VkUtils::bindSetLayout(i, Type, flags[i]));
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(pGPU->device, &layoutInfo, nullptr, &SetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture set layout!");
        }
    }
    void createDescriptorPool() {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (size_t i = 0; i < 1; i++) {
            poolSizes.push_back({ Type, static_cast<uint32_t>(1) });
        }
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(1);

        if (vkCreateDescriptorPool(pGPU->device, &poolInfo, nullptr, &Pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture pool!");
        }
    }
    void createDescriptorSets(VkTexture& texture) {
        std::vector<VkDescriptorSetLayout> layouts(1, SetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(1);
        if (vkAllocateDescriptorSets(pGPU->device, &allocInfo, Sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate texture sets!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites(1);
        for (size_t i = 0; i < 1; i++) {

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.ImageView; // TODO: Generalize
            imageInfo.sampler = texture.Sampler; // TODO: Generalize

            VkWriteDescriptorSet writeSampler = VkUtils::writeDescriptor(0, Type, Sets[i]);
            writeSampler.pImageInfo = &imageInfo;
            descriptorWrites[0] = writeSampler;

            vkUpdateDescriptorSets(pGPU->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
};