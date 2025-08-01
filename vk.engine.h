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

namespace vk {   
    struct Engine : _EngineCPU {
        Engine(GPU* pGPU, Swapchain* pSwapchain) 
            : pGPU(pGPU), pSwapchain(pSwapchain), _EngineCPU(pGPU)
        {        };

        ~Engine() {}

        
        template <int sceneCount, int computeCount>
        void run(Scene(&scene)[sceneCount], ComputePPL(&compute)[computeCount], Pipeline& particlePPL, SSBO& ssbo) {
            std::jthread t1(deltaTime);

            uint32_t currFrame = pSwapchain->currentFrame;

            pSwapchain->vkAquireImage(imageAvailable[currFrame], pSwapchain->imageIndex);
            // Compute Queue
            vkComputeSync(pSwapchain->imageIndex);
            runCompute(compute);
            vkSubmitComputeQueue(pGPU, pSwapchain->imageIndex);

            // Render Queue
            vkRenderSync(pSwapchain->imageIndex);
            runGraphics(scene, particlePPL, ssbo, pSwapchain->imageIndex);
            vkSubmitGraphicsQueue(pGPU, pSwapchain->imageIndex);

            /* Present Image */
            pSwapchain->presentImage(imageCompleted[currFrame], pSwapchain->imageIndex);

            pSwapchain->currentFrame = (currFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            pSwapchain->imageIndex = pSwapchain->currentFrame;
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
        GPU* pGPU;
        Swapchain* pSwapchain;

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
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassInfo
            { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            renderPassInfo.renderPass = pSwapchain->renderPass;
            renderPassInfo.framebuffer = pSwapchain->framebuffers[imageIndex];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = pGPU->extent;
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            VkViewport viewport = createViewPort(pGPU->extent);
            VkRect2D scissor = createScissor({ 0, 0 }, pGPU->extent);

            VkCommandBufferBeginInfo beginInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

            VK_CHECK_RESULT(vkBeginCommandBuffer(renderCommands[pSwapchain->currentFrame], &beginInfo));

            vkCmdBeginRenderPass(renderCommands[pSwapchain->currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdSetViewport(renderCommands[pSwapchain->currentFrame], 0, 1, &viewport);
            vkCmdSetScissor(renderCommands[pSwapchain->currentFrame], 0, 1, &scissor);
        }

        template <int size>
        void runGraphics(Scene(&scene)[size], Pipeline& particlePipeline, SSBO& ssbo, uint32_t& imageIndex) {
            beginRenderPass(imageIndex);

            particlePipeline.bind();
            ssbo.draw();

            for (int i = 0; i < size; i++) {
                scene[i].render(renderCommands[pSwapChain->currentFrame]);
            }

            endRenderPass();
        }

        void endRenderPass() {
            vkCmdEndRenderPass(renderCommands[pSwapchain->currentFrame]);
            VK_CHECK_RESULT(vkEndCommandBuffer(renderCommands[pSwapchain->currentFrame]));
        }

        VkViewport createViewPort(VkExtent2D& extent) {
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)extent.width;
            viewport.height = (float)extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            return viewport;
        }

        VkRect2D createScissor(VkOffset2D offset, VkExtent2D& extent) {
            VkRect2D scissor{};
            scissor.offset = offset;
            scissor.extent = extent;
            return scissor;
        }
    };
}

#endif