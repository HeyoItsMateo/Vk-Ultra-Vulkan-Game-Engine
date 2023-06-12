#include "VulkanGPL.h"

struct VkSyncObjects {
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VkGraphicsUnit* VkGPU;
    VkSyncObjects(VkGraphicsUnit& VkGPU) {
        this->VkGPU = &VkGPU;

        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(VkGPU.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(VkGPU.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(VkGPU.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
    ~VkSyncObjects() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(VkGPU->device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(VkGPU->device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(VkGPU->device, inFlightFences[i], nullptr);
        }
    }
};

struct VkGraphics : VkGraphicsQueue {
    VkCPU cmdUnit;
    VkSyncObjects syncObjects;
    VkGraphics(VkWindow* pWindow) : VkGraphicsQueue(pWindow), syncObjects(*this), cmdUnit(*this) {

    }
    void render(VkGraphicsPipeline pipeLine) {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            pipeLine.ubo.update(currentFrame, swapChainExtent.width, swapChainExtent.height);
            //updateUniformBuffer(currentFrame);
            drawFrame(pipeLine);
        }

        vkDeviceWaitIdle(device);
    }
private:
    uint32_t currentFrame = 0;
    void drawFrame(VkGraphicsPipeline pipeLine) {
        uint32_t imageIndex;
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &syncObjects.imageAvailableSemaphores[currentFrame];
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdUnit.commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &syncObjects.renderFinishedSemaphores[currentFrame];

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        vkUpdateKHR(imageIndex);

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo = VkUtils::vkBeginRenderPass(renderPass, swapChainFramebuffers[imageIndex], swapChainExtent, clearValues);

        vkRenderImage(renderPassInfo, pipeLine, imageIndex);

        

        vkPresentKHR(imageIndex);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    void vkUpdateKHR(uint32_t imageIndex) {
        vkWaitForFences(device, 1, &syncObjects.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, syncObjects.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(device, 1, &syncObjects.inFlightFences[currentFrame]);
    }
    void vkRenderImage(VkRenderPassBeginInfo& renderPassInfo, VkGraphicsPipeline pipeLine, uint32_t imageIndex) { //TODO: rename to vkRenderImages()
        vkResetCommandBuffer(cmdUnit.commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(cmdUnit.commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        vkCmdBeginRenderPass(cmdUnit.commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdUnit.commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(cmdUnit.commandBuffers[currentFrame], 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmdUnit.commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLine.pipelineLayout, 0, 1, &pipeLine.uniformSet.Sets[currentFrame], 0, nullptr);
        vkCmdBindPipeline(cmdUnit.commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLine.graphicsPipeline);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdUnit.commandBuffers[currentFrame], 0, 1, &pipeLine.VBO.buffer, offsets);
        vkCmdBindIndexBuffer(cmdUnit.commandBuffers[currentFrame], pipeLine.EBO.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmdUnit.commandBuffers[currentFrame], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdUnit.commandBuffers[currentFrame]);

        if (vkEndCommandBuffer(cmdUnit.commandBuffers[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
    void vkPresentKHR(uint32_t imageIndex) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &syncObjects.renderFinishedSemaphores[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
};
class VkGraphicsEngine : VkGraphicsPipeline {
public:
    VkCPU cmdUnit;

    VkSyncObjects syncObjects;
    VkGraphicsEngine(VkWindow* pWindow) : VkGraphicsPipeline(pWindow), syncObjects(*this), cmdUnit(*this) {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ubo.update(currentFrame, swapChainExtent.width, swapChainExtent.height);
            //updateUniformBuffer(currentFrame);


            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }
    ~VkGraphicsEngine() {
        
    }
private:
    uint32_t currentFrame = 0;

    void drawFrame() {
        vkWaitForFences(device, 1, &syncObjects.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, syncObjects.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(device, 1, &syncObjects.inFlightFences[currentFrame]);

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo = VkUtils::vkBeginRenderPass(renderPass, swapChainFramebuffers[imageIndex], swapChainExtent, clearValues);

        vkRenderImage(renderPassInfo, imageIndex);

        //VkSemaphore waitSemaphores[] = {  };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        //VkSemaphore signalSemaphores[] = { syncObjects.renderFinishedSemaphores[currentFrame] };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &syncObjects.imageAvailableSemaphores[currentFrame];
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdUnit.commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &syncObjects.renderFinishedSemaphores[currentFrame];

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        //VkSwapchainKHR swapChains[] = { swapChain };
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &syncObjects.renderFinishedSemaphores[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device);
        cleanupSwapChain();

        createSwapChain();
        createImageViews();

        createImageResource<VkColorImage>(color);
        createImageResource<VkDepthImage>(depth);
        createFramebuffers();
    }
    void cleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        destroyImageResource<VkColorImage>(color);
        destroyImageResource<VkDepthImage>(depth);
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }
    void vkRenderImage(VkRenderPassBeginInfo& renderPassInfo, uint32_t imageIndex) { //TODO: rename to vkRenderImages()
        vkResetCommandBuffer(cmdUnit.commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(cmdUnit.commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        vkCmdBeginRenderPass(cmdUnit.commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdUnit.commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(cmdUnit.commandBuffers[currentFrame], 0, 1, &scissor);

        VkDescriptorSet sets[] = { uniformSet.Sets[currentFrame], textureSet.Sets[currentFrame], storageSet.Sets[currentFrame] };

        vkCmdBindDescriptorSets(cmdUnit.commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, sets, 0, nullptr);
        vkCmdBindPipeline(cmdUnit.commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdUnit.commandBuffers[currentFrame], 0, 1, &VBO.buffer, offsets);
        vkCmdBindIndexBuffer(cmdUnit.commandBuffers[currentFrame], EBO.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmdUnit.commandBuffers[currentFrame], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdUnit.commandBuffers[currentFrame]);

        if (vkEndCommandBuffer(cmdUnit.commandBuffers[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
};