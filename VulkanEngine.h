#include "VulkanGPL.h"


void userInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{// Sets Keyboard Commands
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

struct VkGraphicsEngine : VkSwapChain {
public:
    VkCPU cmdUnit;

    VkSyncObjects syncObjects;
    VkGraphicsEngine(VkWindow& pWindow) : VkSwapChain(pWindow), syncObjects(*this), cmdUnit(*this) {
    }
    void run(VkGraphicsPipeline& pipeline) {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            glfwSetKeyCallback(window, userInput);
            pipeline.uniforms.camera.update(window, Extent, 45.0f, 0.1f, 10.0f);
            pipeline.ubo.update(currentFrame, pipeline.uniforms.camera);
            //updateUniformBuffer(currentFrame);

            drawFrame(pipeline);
        }

        vkDeviceWaitIdle(device);
    }
    ~VkGraphicsEngine() {
        
    }
private:
    uint32_t currentFrame = 0;

    void drawFrame(VkGraphicsPipeline& pipeline) {
        vkWaitForFences(device, 1, &syncObjects.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChainKHR, UINT64_MAX, syncObjects.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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

        VkRenderPassBeginInfo renderPassInfo = VkUtils::vkBeginRenderPass(renderPass, swapChainFramebuffers[imageIndex], Extent, clearValues);
        
        vkResetCommandBuffer(cmdUnit.commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(cmdUnit.commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        vkCmdBeginRenderPass(cmdUnit.commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkRenderImage(pipeline, imageIndex);

        //VkSwapchainKHR swapChains[] = { swapChain };
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &syncObjects.renderFinishedSemaphores[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChainKHR;
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
    void vkRenderImage(VkGraphicsPipeline& pipeline, uint32_t imageIndex) { //TODO: rename to vkRenderImages()
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)Extent.width;
        viewport.height = (float)Extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdUnit.commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = Extent;
        vkCmdSetScissor(cmdUnit.commandBuffers[currentFrame], 0, 1, &scissor);

        // Pipeline binding
        VkDescriptorSet sets[] = { pipeline.uniformSet.Sets[currentFrame], pipeline.textureSet.Sets[currentFrame], pipeline.storageSet.Sets[currentFrame] };
        vkCmdBindDescriptorSets(cmdUnit.commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 3, sets, 0, nullptr);
        vkCmdBindPipeline(cmdUnit.commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.graphicsPipeline);
        
        // Vertex binding (objects to draw)
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdUnit.commandBuffers[currentFrame], 0, 1, &pipeline.VBO.buffer, offsets);
        vkCmdBindIndexBuffer(cmdUnit.commandBuffers[currentFrame], pipeline.EBO.buffer, 0, VK_INDEX_TYPE_UINT16);

        // Draw Command
        vkCmdDrawIndexed(cmdUnit.commandBuffers[currentFrame], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdUnit.commandBuffers[currentFrame]);

        if (vkEndCommandBuffer(cmdUnit.commandBuffers[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

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
    }
};