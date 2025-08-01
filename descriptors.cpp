#include "descriptors.h"

#include <vector>

namespace vk {
    void Descriptor::createDescriptorPool(VkDescriptorType& type, uint32_t bindingCount)
    {
        VkDescriptorPoolSize poolSizes
        { type , static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * bindingCount };

        VkDescriptorPoolCreateInfo poolInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSizes;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &Pool));
    }

    void Descriptor::createDescriptorSetLayout(VkDescriptorType& type, VkShaderStageFlags& flag, uint32_t bindingCount)
    {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(bindingCount);
        for (uint32_t i = 0; i < bindingCount; i++) {
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

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &SetLayout));
    }

    void Descriptor::allocateDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, SetLayout);

        VkDescriptorSetAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(MAX_FRAMES_IN_FLIGHT);

        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, Sets.data()));
    }
}