#ifndef hTestFile
#define hTestFile


template<typename VkHandle>
using VkFunction = void(*)(VkDevice, VkHandle, const VkAllocationCallbacks*);

template<typename createInfo, typename VkHandle>
using VkCreate = VkResult(*)(VkDevice, const createInfo*, const VkAllocationCallbacks*, VkHandle*);


template<typename VkHandle, typename VkCreateInfo, VkCreate<VkCreateInfo, VkHandle> vkCreate, VkFunction<VkHandle> vkDestroy>
struct VkObject {
    VkHandle mHandle;
    VkObject(VkCreateInfo& createInfo) {
        
        if (vkCreate(VkGPU::device, &createInfo, nullptr, &mHandle) != VK_SUCCESS) {
            throw std::runtime_error("failed to create '" + std::string(typeid(mHandle).name()) + "'!");
        }
    }
    ~VkObject() {
        vkDestroy(VkGPU::device, mHandle, nullptr);
    }
};


template<typename VkHandle>
using VkMemoryFunction = void(*)(VkDevice, VkHandle, VkMemoryRequirements*);



#endif