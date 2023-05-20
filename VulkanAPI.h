#include "appWindow.h"

#include <iostream>
#include <vector>


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct debugAPI {
	debugAPI() 
	{
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
	}
private:
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        // Store the number of available validation layers
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }
};

struct VulkanAPI {
	VulkanAPI() {

	}
};