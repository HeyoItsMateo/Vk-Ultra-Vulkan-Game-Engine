#include "vk.instance.h"

#include <limits>
#include <algorithm>
#include <optional>
#include <set>

const int MAX_FRAMES_IN_FLIGHT = 1;

namespace vk {
    struct GPU : Instance {
        GPU();
        ~GPU();
    public:
        inline static VkDevice device;
        inline static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        inline static VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        inline static std::optional<uint32_t> graphicsFamily;
        inline static VkQueue graphicsQueue;
        inline static VkQueue computeQueue;

        inline static std::optional<uint32_t> presentFamily;
        inline static VkQueue presentQueue;

        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
        inline static VkExtent2D Extent;

        static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    protected:
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    private:
        void pickPhysicalDevice();
        void createPhysicalDevice();

        bool isDeviceSuitable(VkPhysicalDevice device);
        bool findQueueFamilies(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        void querySwapChainSupport(VkPhysicalDevice device);

        void getSampleCount();
        void getSwapExtent();
    };
}
