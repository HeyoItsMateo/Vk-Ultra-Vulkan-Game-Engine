#pragma once
#ifndef hSwapChain
#define hSwapChain

#include "vk.gpu.h"
#include "vk.image.h"

namespace vk {
    inline static double dt;
    inline static bool time = true;

    struct SwapChain : GPU {
        SwapChain();
        ~SwapChain();
    public:
        inline static double lastTime = 0.0;

        inline static VkSwapchainKHR swapChainKHR;
        inline static VkRenderPass renderPass;
        inline static uint32_t currentFrame = 0;

        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> framebuffers;
        uint32_t mipLevels = 1;

        Color color;
        Depth depth;

        void recreateSwapChain();
        
    protected:
        void deltaTime() {
            dt = (glfwGetTime() - lastTime);
            lastTime = glfwGetTime();
        }
    private:
        void createSwapChain();
        void createImageViews();
        void createFramebuffers();
        void createRenderPass();

        void cleanupSwapChain();
        
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }
        
    };
}
#endif