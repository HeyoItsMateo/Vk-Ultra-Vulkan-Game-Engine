#include "vk.gpu.h"

namespace vk {
    /* Graphics Processing Unit */
    GPU::GPU(Instance* instance)
        : pInstance(instance), buffer(this)
    {
        pickPhysicalDevice(this);
        createPhysicalDevice(this);
    }
    GPU::~GPU()
    {
        vkDestroyDevice(device, nullptr);
    }
    //Public:
    uint32_t GPU::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
    //Private:
    void GPU::pickPhysicalDevice(vk::GPU* pGPU)
    {
        uint32_t deviceCount = 0;
        VkInstance instance = pGPU->pInstance->instance;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {// Debug failure to find GPU with Vulkan support
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        VkSurfaceKHR surface = pGPU->pInstance->surface;
        for (const auto& device : devices)
        {// Pick most optimal GPU out of available devices
            if (isDeviceSuitable(pGPU, device, surface))
            {// Set device and specifications
                pGPU->physicalDevice = device;
                pGPU->msaaSamples = getSampleCount(pGPU->physicalDevice);
                //TODO: figure out how to make this work for windowless rendering/compute as well
                pGPU->extent = getSwapExtent(pGPU->pInstance->pWindow->handle, pGPU ->capabilities);
                break;
            }
        }

        if (pGPU->physicalDevice == VK_NULL_HANDLE)
        {// Debug failure to find/set GPU
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }
    void GPU::createPhysicalDevice(GPU* pGPU)
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { pGPU->graphicsFamily.value(), pGPU->presentFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo
            { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.fillModeNonSolid = VK_TRUE;
        deviceFeatures.shaderFloat64 = VK_TRUE;
        deviceFeatures.geometryShader = VK_TRUE;
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading feature for the device

        VkDeviceCreateInfo createInfo
        { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(Instance::validationLayers.size());
            createInfo.ppEnabledLayerNames = Instance::validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        VK_CHECK_RESULT(vkCreateDevice(pGPU->physicalDevice, &createInfo, nullptr, &pGPU->device));

        vkGetDeviceQueue(pGPU->device, pGPU->graphicsFamily.value(), 0, &pGPU->graphicsQueue);
        vkGetDeviceQueue(pGPU->device, pGPU->graphicsFamily.value(), 0, &pGPU->computeQueue);
        vkGetDeviceQueue(pGPU->device, pGPU->presentFamily.value(), 0, &pGPU->presentQueue);

    }

    bool GPU::isDeviceSuitable(GPU* pGPU, const VkPhysicalDevice device, const VkSurfaceKHR surface)
    {
        bool queueFamilySupported = findQueueFamilies(pGPU, device, surface);
        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;

        if (extensionsSupported) {
            querySwapChainSupport(pGPU, device, surface);
            swapChainAdequate = !pGPU->formats.empty() && !pGPU->presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        return queueFamilySupported && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool GPU::findQueueFamilies(const GPU* pGPU, const VkPhysicalDevice device, const VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                pGPU->graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                pGPU->presentFamily = i;
            }

            if (pGPU->graphicsFamily.has_value() && pGPU->presentFamily.has_value()) {
                return true;
            }
            i++;
        }
        return false;
    }
    //TODO: decide if and how I want to restructure the "requiredExtentions" to just include all available extensions
    bool GPU::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void GPU::querySwapChainSupport(vk::GPU* pGPU, const VkPhysicalDevice device, const VkSurfaceKHR surface)
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &pGPU->capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            pGPU->formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, pGPU->formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            pGPU->presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, pGPU->presentModes.data());
        }
    }

    VkSampleCountFlagBits GPU::getSampleCount(VkPhysicalDevice& physicalDevice)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
        else { return VK_SAMPLE_COUNT_1_BIT; }
    }

    VkExtent2D GPU::getSwapExtent(GLFWwindow* handle, VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(handle, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            return {
                std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };
        }
    }
}