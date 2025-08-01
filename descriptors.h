#ifndef hDescriptors
#define hDescriptors

#ifndef hInstance
    #include "vk.instance.h"
#endif

namespace vk {
    struct Descriptor {
        Descriptor(VkDevice device, VkDescriptorType type, VkShaderStageFlags flag, uint32_t bindingCount = 1)
            : device(device)
        {
            createDescriptorPool(type, bindingCount);
            createDescriptorSetLayout(type, flag, bindingCount);
            allocateDescriptorSets();
        }
        ~Descriptor() {
            if (Pool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, Pool, nullptr);
            }
            if (SetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(device, SetLayout, nullptr);
            }
        }
    public:
        VkDescriptorSetLayout SetLayout;
        std::vector<VkDescriptorSet> Sets;

    protected:
        VkDevice device;
        VkDescriptorPool Pool;
        virtual void writeDescriptorSets(uint32_t bindingCount) {}

    private:

        void createDescriptorPool(VkDescriptorType& type, uint32_t bindingCount);

        void createDescriptorSetLayout(VkDescriptorType& type, VkShaderStageFlags& flag, uint32_t bindingCount);

        void allocateDescriptorSets();
    };
    /**/
    struct Descriptor_ {
        VkDescriptorType type;
        VkShaderStageFlags stageFlags;
        uint32_t bindingCount = 1;
        //uint32_t mipLevels;
    private:
        //friend void DescriptorSet::writeSet(Descriptor_* pDescriptors, uint32_t size);
        friend struct DescriptorSet;
        virtual VkWriteDescriptorSet writeDescriptor(VkDescriptorSet& dstSet, uint32_t dstBinding)
        {
            return VkWriteDescriptorSet{};
        };
        
    };

    /*
    struct DescriptorSet {
        template<uint32_t descriptorCount>
        DescriptorSet(Descriptor_(&descriptors)[descriptorCount]) {
            size = descriptorCount;
            pDescriptors = descriptors;
            
            createSetLayout();
            createSet();
        }
        ~DescriptorSet() {
            vkDestroyDescriptorSetLayout(GPU::device, layout, nullptr);
        }
    public:
        VkDescriptorSet set;
        VkDescriptorSetLayout layout;
        std::vector<VkDescriptorPoolSize> setSize;
        void writeSet() {
            std::vector<VkWriteDescriptorSet> descriptorWrites(size);
            for (int i = 0; i < size; i++) {
                descriptorWrites[i] = pDescriptors[i].writeDescriptor(set, i);
            }
            vkUpdateDescriptorSets(GPU::device, size, descriptorWrites.data(), 0, nullptr);
        }
    private:
        friend struct DescriptorPool_;
        Descriptor_* pDescriptors;
        uint32_t size;
        void createSet() {
            setSize.resize(size);
            for (int i = 0; i < size; i++) {
                setSize[i] = { pDescriptors[i].type , static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * pDescriptors[i].bindingCount };
            }
        }
        void createSetLayout() {
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings(size);
            for (uint32_t i = 0; i < size; i++) {
                layoutBindings[i].binding = i;
                layoutBindings[i].descriptorCount = 1;
                layoutBindings[i].descriptorType = pDescriptors[i].type;
                layoutBindings[i].pImmutableSamplers = nullptr;
                layoutBindings[i].stageFlags = pDescriptors[i].stageFlags;
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo
            { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            layoutInfo.bindingCount = size;
            layoutInfo.pBindings = layoutBindings.data();

            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(GPU::device, &layoutInfo, nullptr, &layout));
        }
        
    };

    struct DescriptorPool_ {
        template<uint32_t setCount>
        DescriptorPool_(DescriptorSet(&DescriptorSets)[setCount]) {
            createDescriptorPool(DescriptorSets, setCount);
            allocateDescriptorSets(DescriptorSets, setCount);
            writeDescriptorSets(DescriptorSets, setCount);
        }
        ~DescriptorPool_() {
            vkDestroyDescriptorPool(GPU::device, Pool, nullptr);
        }
    public:
        std::vector<VkDescriptorSet> Sets;
    private:
        VkDescriptorPool Pool;
        void createDescriptorPool(std::vector<DescriptorSet>& DescriptorSets, uint32_t size) {
            std::vector<VkDescriptorPoolSize> poolSizes{};
            for (int i = 0; i < size; i++) {
                poolSizes.insert(std::end(poolSizes), std::begin(DescriptorSets[i].setSize), std::end(DescriptorSets[i].setSize));
            }

            VkDescriptorPoolCreateInfo poolInfo
            { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
            poolInfo.poolSizeCount = size;
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = size; //if MAX_FRAMES_IN_FLIGHT == 1, then there will be only 1 set.
            // ^number of sets that can be allocated by vkAllocateDescriptorSets()
            VK_CHECK_RESULT(vkCreateDescriptorPool(GPU::device, &poolInfo, nullptr, &Pool));
        }
        void allocateDescriptorSets(std::vector<DescriptorSet>& DescriptorSets, uint32_t size) {
            std::vector<VkDescriptorSetLayout> layouts(size, DescriptorSets.data()->layout);

            VkDescriptorSetAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            allocInfo.descriptorPool = Pool;
            allocInfo.descriptorSetCount = size;
            allocInfo.pSetLayouts = layouts.data();

            VK_CHECK_RESULT(vkAllocateDescriptorSets(GPU::device, &allocInfo, &DescriptorSets.data()->set));
        }
        void writeDescriptorSets(std::vector<DescriptorSet>& DescriptorSets, uint32_t size) {
            for (int i = 0; i < size; i++) {
                DescriptorSets[i].writeSet();
            }
        }
    };
    */
}

#endif