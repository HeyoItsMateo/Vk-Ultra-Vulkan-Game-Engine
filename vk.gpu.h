#pragma once
#ifndef hGPU
#define hGPU

#include "vk.instance.h"

#include <limits>
#include <algorithm>
#include <optional>
#include <set>

namespace vk {
    struct GPU {
        GPU(Instance* vkInstance);
        ~GPU();
    public:
        Instance* pInstance;

        VkDevice device;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        
        // Decide if I want to make these into objects to reduce memory transfer/access
        mutable std::optional<uint32_t> graphicsFamily;
        VkQueue graphicsQueue;
        VkQueue computeQueue;
        // Decide if I want to make these into objects to reduce memory transfer/access
        mutable std::optional<uint32_t> presentFamily;
        std::vector<VkPresentModeKHR> presentModes;
        VkQueue presentQueue;

        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        VkExtent2D extent;
        
        //TODO: figure out how to cache the result of this
        static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    protected:
        inline static std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    private:
        /*---GPU Device Creation Functions---*/
        static void pickPhysicalDevice(GPU* pGPU);
        static void createPhysicalDevice(GPU* pGPU);

        static bool isDeviceSuitable(GPU* pGPU, const VkPhysicalDevice device, const VkSurfaceKHR surface);
        static bool findQueueFamilies(const GPU* pGPU, const VkPhysicalDevice device, const VkSurfaceKHR surface);
        static bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        static void querySwapChainSupport(GPU* pGPU, const VkPhysicalDevice device, const VkSurfaceKHR surface);

        static VkSampleCountFlagBits getSampleCount(VkPhysicalDevice& physicalDevice);
        static VkExtent2D getSwapExtent(GLFWwindow* handle, VkSurfaceCapabilitiesKHR& capabilities);
    };

    struct GPU_Object {
        GPU_Object(GPU* const pHost) : pHost(pHost) {};
    protected:
        GPU* pHost;
    };
}
#endif