#ifndef hDescriptors
#define hDescriptors

#include <set>

namespace vk {
    struct Descriptor {
        VkDescriptorSetLayout SetLayout;
        std::vector<VkDescriptorSet> Sets;
        Descriptor(VkDescriptorType type, VkShaderStageFlags flag, uint32_t bindingCount = 1) {
            createDescriptorPool(type, bindingCount);
            createDescriptorSetLayout(type, flag, bindingCount);
            allocateDescriptorSets();
        }
        ~Descriptor() {
            vkDestroyDescriptorPool(GPU::device, Pool, nullptr);
            vkDestroyDescriptorSetLayout(GPU::device, SetLayout, nullptr);
        }
    protected:
        VkDescriptorPool Pool;
        virtual void writeDescriptorSets(uint32_t bindingCount) {}
    private:
        void createDescriptorPool(VkDescriptorType& type, uint32_t bindingCount) {
            VkDescriptorPoolSize poolSizes
            { type , static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * bindingCount };

            VkDescriptorPoolCreateInfo poolInfo
            { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSizes;
            poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            if (vkCreateDescriptorPool(GPU::device, &poolInfo, nullptr, &Pool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor pool!");
            }

        }
        void createDescriptorSetLayout(VkDescriptorType& type, VkShaderStageFlags& flag, uint32_t bindingCount) {
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

            if (vkCreateDescriptorSetLayout(GPU::device, &layoutInfo, nullptr, &SetLayout) != VK_SUCCESS) {
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
            if (vkAllocateDescriptorSets(GPU::device, &allocInfo, Sets.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }
        }
    };
}





#endif