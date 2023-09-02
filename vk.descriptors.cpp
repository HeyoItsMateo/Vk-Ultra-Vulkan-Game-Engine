#include "vk.descriptors.h"

vk::descriptor::descriptor(VkDescriptorType type, VkShaderStageFlags flag, uint32_t bindingCount)
{
    Type = type;
    createDescriptorPool(type, bindingCount);
    createDescriptorSetLayout(type, flag, bindingCount);
    allocateDescriptorSets();
}