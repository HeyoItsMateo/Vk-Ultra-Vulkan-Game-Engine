#ifndef hDescriptors
#define hDescriptors

struct VkDescriptor {
    VkDescriptorSetLayout SetLayout;
    std::vector<VkDescriptorSet> Sets;
    VkDescriptor(VkDescriptorType type, VkShaderStageFlags flag, uint32_t bindingCount) {
        Type = type;
        createDescriptorPool(type, bindingCount);
        createDescriptorSetLayout(type, flag, bindingCount);
        allocateDescriptorSets();
    }
    ~VkDescriptor() {
        vkDestroyDescriptorPool(VkGPU::device, Pool, nullptr);
        vkDestroyDescriptorSetLayout(VkGPU::device, SetLayout, nullptr);
    }
protected:
    VkDescriptorType Type;
    VkDescriptorPool Pool;
    template <typename T>
    inline VkWriteDescriptorSet writeSet(std::vector<T>& bufferInfo, std::array<size_t,2> dst) {
        VkWriteDescriptorSet writeInfo
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeInfo.dstSet = Sets[dst[0]];
        writeInfo.dstBinding = dst[1];
        writeInfo.dstArrayElement = 0;

        writeInfo.descriptorType = Type;
        writeInfo.descriptorCount = 1;

        if constexpr (std::is_same<T, VkDescriptorImageInfo>::value) {
            writeInfo.pImageInfo = &bufferInfo[dst[1]];
        }
        else {
            writeInfo.pBufferInfo = &bufferInfo[dst[1]];
        }
        
        return writeInfo;
    }
private:
    void createDescriptorPool(VkDescriptorType& type, uint32_t bindingCount) {
        VkDescriptorPoolSize poolSizes
        { type , static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * bindingCount };

        VkDescriptorPoolCreateInfo poolInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSizes;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(VkGPU::device, &poolInfo, nullptr, &Pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

    }
    void createDescriptorSetLayout(VkDescriptorType& type, VkShaderStageFlags& flag, uint32_t bindingCount) {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(bindingCount);
        for (size_t i = 0; i < bindingCount; i++) {
            layoutBindings[i].binding = i;
            layoutBindings[i].descriptorCount = 1;
            layoutBindings[i].descriptorType = type;
            layoutBindings[i].pImmutableSamplers = nullptr;
            layoutBindings[i].stageFlags = flag;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = bindingCount;
        layoutInfo.pBindings = layoutBindings.data();

        if (vkCreateDescriptorSetLayout(VkGPU::device, &layoutInfo, nullptr, &SetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
    void allocateDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, SetLayout);

        VkDescriptorSetAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(VkGPU::device, &allocInfo, Sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }
};

struct VkTextureSet : VkDescriptor {
    VkDescriptorType mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkShaderStageFlagBits mFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkTextureSet(VkTexture& texture) : VkDescriptor(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1) {
        writeDescriptorSets(texture, 1);
    }

    void writeDescriptorSets(VkTexture& texture, uint32_t bindingCount = 1) {
        std::vector<VkDescriptorImageInfo> imageInfo(bindingCount);
        std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            for (size_t j = 0; j < bindingCount; j++) {
                imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo[j].imageView = texture.ImageView; // TODO: Generalize
                imageInfo[j].sampler = texture.Sampler; // TODO: Generalize
                descriptorWrites[j] = writeSet(imageInfo, { i,j });
            }
            vkUpdateDescriptorSets(VkGPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    }
};


#endif