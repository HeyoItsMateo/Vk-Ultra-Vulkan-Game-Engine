#pragma once
#ifndef hEngine
#define hEngine

#include "vk.swapchain.h"
#include "vk.cpu.h"

#include "vk.graphics.h"
#include "vk.compute.h"
#include "Scene.h"

#include "vk.ubo.h"
#include "vk.ssbo.h"

#include <chrono>

struct RenderPass {
    RenderPass(VkRenderPass renderPass, VkExtent2D extent) {
        _clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        _clearValues[1].depthStencil = { 1.0f, 0 };

        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.pNext = nullptr;
        info.renderPass = renderPass;
        info.renderArea.offset = { 0, 0 };
        info.renderArea.extent = extent;

        setClearValues(_clearValues);
    }
public:
    VkRenderPassBeginInfo info;

    void setFramebuffer(VkFramebuffer framebuffer) {
        info.framebuffer = framebuffer;
    }
    void setExtent(VkExtent2D extent) {
        info.renderArea.extent = extent;
    }
    void setClearValues(std::array<VkClearValue, 2>& clearValues) {
        info.clearValueCount = static_cast<uint32_t>(clearValues.size());
        info.pClearValues = clearValues.data();
    }
    void setRenderPass(VkRenderPass renderPass) {
        info.renderPass = renderPass;
    }
private:
    std::array<VkClearValue, 2> _clearValues{};
};

namespace vk {   
    struct Engine : SwapChain, EngineCPU {
        Engine() {
            pRenderPass = new RenderPass(renderPass, Extent);
        }
        ~Engine() {
            delete pRenderPass;
        }
    public:
        uint32_t imageIndex = 0;
        RenderPass* pRenderPass;

        template <int sceneCount, int computeCount>
        void run(Scene(&scene)[sceneCount], ComputePPL(&compute)[computeCount], Pipeline& particlePPL, SSBO& ssbo) {
            std::jthread t1(&Engine::deltaTime, this);

            vkAquireImage(imageAvailable[currentFrame], imageIndex);

            // Compute Queue
            vkComputeSync();
            runCompute(compute);
            vkSubmitComputeQueue();

            // Render Queue
            vkRenderSync();
            runGraphics(scene, particlePPL, ssbo, imageIndex);
            vkSubmitGraphicsQueue();

            /* Present Image */
            vkPresentImage(imageCompleted[currentFrame], imageIndex);

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }
    protected:
        void updateVtx() {
            /*
            float cyclicTime = glm::radians(45 * float(lastTime));
            testVtx[0].pos[1] = 0.5f * glm::sin(cyclicTime);
            testVtx[1].pos[1] = 0.5f * glm::cos(cyclicTime);
            testVtx[2].pos[1] = 0.5f * glm::sin(cyclicTime);
            testVtx[3].pos[1] = 0.5f * glm::cos(cyclicTime);

            testVtx[4].pos = { -0.5 - glm::abs(glm::sin(cyclicTime)), -0.5f,  0.5 + glm::abs(glm::sin(cyclicTime)) };
            testVtx[5].pos = {  0.5 + glm::abs(glm::sin(cyclicTime)), -0.5f,  0.5 + glm::abs(glm::sin(cyclicTime)) };
            testVtx[6].pos = {  0.5 + glm::abs(glm::sin(cyclicTime)), -0.5f, -0.5 - glm::abs(glm::sin(cyclicTime)) };
            testVtx[7].pos = { -0.5 - glm::abs(glm::sin(cyclicTime)), -0.5f, -0.5 - glm::abs(glm::sin(cyclicTime)) };
            */
        }
        void userInput(GLFWwindow* window, int key, int scancode, int action, int mods)
        {// Sets Keyboard Commands
            //TODO: map
            switch (action)
            {// Checks for user keypress
            case GLFW_PRESS:
                switch (key)
                {// Checks for keypress type and returns corresponding action
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, true);
                    break;
                case GLFW_KEY_C:
                    //wireframe = !wireframe;
                    break;
                case GLFW_KEY_V:
                    //vSync = !vSync;
                    //glfwSwapInterval(vSync);
                    break;
                }
                break;
            default:
                break;
            }
        }
    private:

        void vkAquireImage(VkSemaphore& waitSemaphore, uint32_t imageIndex) {
            VkResult result = vkAcquireNextImageKHR(device, swapChainKHR, UINT64_MAX, waitSemaphore, VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                recreateSwapChain();

                pRenderPass->setExtent(Extent);

                return;
            }
            else if (result != VK_SUBOPTIMAL_KHR && result != VK_SUCCESS) {
                VK_CHECK_RESULT(result);
            }
            
        }
        
        template <int size>
        void runCompute(ComputePPL(&compute)[size]) {
            VkCommandBufferBeginInfo beginInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

            VK_CHECK_RESULT(vkBeginCommandBuffer(computeCommands[currentFrame], &beginInfo));
            for (int i = 0; i < size; i++) {
                compute[i].dispatch();
            }
            VK_CHECK_RESULT(vkEndCommandBuffer(computeCommands[currentFrame]));
        }

        void beginRenderPass(uint32_t& imageIndex) {
            // Update Render pass's framebuffer
            pRenderPass->setFramebuffer(framebuffers[imageIndex]);

            VkViewport viewport = createViewPort();
            VkRect2D scissor = createScissor({ 0, 0 });

            VkCommandBufferBeginInfo beginInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

            VK_CHECK_RESULT(vkBeginCommandBuffer(renderCommands[currentFrame], &beginInfo));

            vkCmdBeginRenderPass(renderCommands[currentFrame], &pRenderPass->info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdSetViewport(renderCommands[currentFrame], 0, 1, &viewport);
            vkCmdSetScissor(renderCommands[currentFrame], 0, 1, &scissor);
        }
        template <int size>
        void runGraphics(Scene(&scene)[size], Pipeline& particlePipeline, SSBO& ssbo, uint32_t& imageIndex) {
            beginRenderPass(imageIndex);

            particlePipeline.bind();
            ssbo.draw();

            for (int i = 0; i < size; i++) {
                scene[i].render();
            }

            endRenderPass();
        }
        void endRenderPass() {
            vkCmdEndRenderPass(renderCommands[currentFrame]);
            VK_CHECK_RESULT(vkEndCommandBuffer(renderCommands[currentFrame]));
        }

        
        void vkPresentImage(VkSemaphore& waitSemaphore, uint32_t& imageIndex) {
            VkPresentInfoKHR presentInfo
            { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
            presentInfo.pNext = NULL;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChainKHR;
            presentInfo.pImageIndices = &imageIndex;

            if (waitSemaphore != VK_NULL_HANDLE) {
                presentInfo.pWaitSemaphores = &waitSemaphore;
                presentInfo.waitSemaphoreCount = 1;
            }

            VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Window::framebufferResized) {
                Window::framebufferResized = false;
                recreateSwapChain();

                pRenderPass->setExtent(Extent);

                return;
            }
            else {
                VK_CHECK_RESULT(result);
            }
        }

        //TODO: remake into objects to reduce constant operations
        VkViewport createViewPort(VkExtent2D& extent = Extent) {
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)extent.width;
            viewport.height = (float)extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            return viewport;
        }
        VkRect2D createScissor(VkOffset2D offset, VkExtent2D& extent = Extent) {
            VkRect2D scissor{};
            scissor.offset = offset;
            scissor.extent = extent;
            return scissor;
        }
    };
}

#endif