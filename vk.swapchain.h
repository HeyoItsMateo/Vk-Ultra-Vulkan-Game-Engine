#pragma once
#ifndef hSwapChain
#define hSwapChain

#include "vk.gpu.h"
#include "vk.image.h"

namespace vk {
    inline static double lastTime = 0.0;
    inline static double dt;
    inline static bool time = true;
    

    static void deltaTime() {
        dt = (glfwGetTime() - lastTime);
        lastTime = glfwGetTime();
    }

    struct Swapchain {
        Swapchain(GPU* const pGPU);
        ~Swapchain();
    public:
        //TODO: make an array of GPU pointers for multiple swapchains
        inline static GPU* pGPU;
        inline static VkSwapchainKHR KHR;//TODO: array for multiple swapchains
        inline static VkRenderPass renderPass;//TODO: array for multiple render passes
        inline static uint32_t currentFrame = 0;
        uint32_t imageIndex = 0;

        std::vector<VkImage> images;
        std::vector<VkImageView> ImageViews;
        // TODO: Decide if I want to make this an object
        std::vector<VkFramebuffer> framebuffers;
        uint32_t mipLevels = 1;

        static Color color;
        static Depth depth;

        static void recreate(Swapchain* const pSwapchain);

        void vkAquireImage(VkSemaphore& waitSemaphore, uint32_t& imageIndex) {
            VkResult result = vkAcquireNextImageKHR(pGPU->device, KHR, UINT64_MAX, waitSemaphore, VK_NULL_HANDLE, &imageIndex);

            validateImage(result, this);
        }

        void presentImage(VkSemaphore& waitSemaphore, uint32_t& imageIndex) {
            VkPresentInfoKHR presentInfo
            { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
            presentInfo.pNext = NULL;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &KHR;
            presentInfo.pImageIndices = &imageIndex;

            if (waitSemaphore != VK_NULL_HANDLE) {
                presentInfo.pWaitSemaphores = &waitSemaphore;
                presentInfo.waitSemaphoreCount = 1;
            }
            validateKHR(vkQueuePresentKHR(pGPU->presentQueue, &presentInfo));
        }

        void validateKHR(VkResult&& result) {
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || pGPU->pInstance->pWindow->framebufferResized) {
                pGPU->pInstance->pWindow->framebufferResized = false;
                recreate(this);
                return;
            }
            else {
                VK_CHECK_RESULT(result);
            }
        }

        static void validateImage(VkResult result, Swapchain* const pSwapchain) {
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                recreate(pSwapchain);
                return;
            }
            else if (result != VK_SUBOPTIMAL_KHR && result != VK_SUCCESS) {
                VK_CHECK_RESULT(result);
            }
        }

    private:
        static void create(Swapchain* const pSwapchain);
        static void createImageViews(Swapchain* const pSwapchain);
        static void createFramebuffers(Swapchain* const pSwapchain);
        void createRenderPass(const VkDevice device);

        static void cleanup(Swapchain* const swapchain);
        
        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }
        static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
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