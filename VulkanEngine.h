#include "VulkanGPL.h"

struct VkGraphicsEngine : VkSwapChain, VkEngineCPU {
    std::vector<Vertex> testVtx = {
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    template<typename T, typename Q>
    inline void run(GraphicsPipeline& pipeline0, GraphicsPipeline& pipeline1, GameObject& gameObject0, GameObject& gameObject1, VkParticlePipeline& particlePipeline, VkComputePipeline& computePipeline, SSBO<T>& ssbo, UBO<Q>& ubo) {
        while (!glfwWindowShouldClose(VkWindow::window)) {
            glfwPollEvents();
            std::jthread t1(glfwSetKeyCallback, VkWindow::window, userInput);
            std::jthread t2(&VkGraphicsEngine::deltaTime, this);

            ubo.update();

            uint32_t imageIndex;
            vkAquireImage(imageIndex);

            // Compute Queue
            runCompute(computePipeline);

            // Render Queue
            renderScene(pipeline0, pipeline1, gameObject0, gameObject1, particlePipeline, ssbo, imageIndex);

            vkSubmitGraphicsQueue();
            vkPresentImage(imageIndex);

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }
        vkDeviceWaitIdle(device);
    }
protected:
    void updateVtx() {
        float cyclicTime = glm::radians(45 * float(lastTime));
        testVtx[0].pos[1] = 0.5f * glm::sin(cyclicTime);
        testVtx[1].pos[1] = 0.5f * glm::cos(cyclicTime);
        testVtx[2].pos[1] = 0.5f * glm::sin(cyclicTime);
        testVtx[3].pos[1] = 0.5f * glm::cos(cyclicTime);

        testVtx[4].pos = { -0.5 - glm::abs(glm::sin(cyclicTime)), -0.5f,  0.5 + glm::abs(glm::sin(cyclicTime)) };
        testVtx[5].pos = {  0.5 + glm::abs(glm::sin(cyclicTime)), -0.5f,  0.5 + glm::abs(glm::sin(cyclicTime)) };
        testVtx[6].pos = {  0.5 + glm::abs(glm::sin(cyclicTime)), -0.5f, -0.5 - glm::abs(glm::sin(cyclicTime)) };
        testVtx[7].pos = { -0.5 - glm::abs(glm::sin(cyclicTime)), -0.5f, -0.5 - glm::abs(glm::sin(cyclicTime)) };
    }

private:
    void runCompute(VkComputePipeline& pipeline) {
        vkComputeSync();
        pipeline.run();
        vkSubmitComputeQueue();
    }
    template<typename T>
    inline void renderScene(GraphicsPipeline& pipeline0, GraphicsPipeline& pipeline1, GameObject& gameObject0, GameObject& gameObject1, VkParticlePipeline& particlePipeline, SSBO<T>& ssbo, uint32_t& imageIndex) {
        vkRenderSync();
        beginRender(imageIndex);

        particlePipeline.bind();
        particlePipeline.draw(ssbo.Buffer[currentFrame]);
       
        pipeline0.bind();
        gameObject0.draw();
        
        pipeline1.bind();
        gameObject1.draw();

        endRender();
    }
    void beginRender(uint32_t& imageIndex) {
        VkCommandBufferBeginInfo beginInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

        if (vkBeginCommandBuffer(renderCommands[currentFrame], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo
        { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = Extent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)Extent.width;
        viewport.height = (float)Extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = Extent;

        vkCmdBeginRenderPass(renderCommands[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(renderCommands[currentFrame], 0, 1, &viewport);
        vkCmdSetScissor(renderCommands[currentFrame], 0, 1, &scissor);
    }
    void endRender() {
        vkCmdEndRenderPass(renderCommands[currentFrame]);
        if (vkEndCommandBuffer(renderCommands[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void vkAquireImage(uint32_t& imageIndex) {
        VkResult result = vkAcquireNextImageKHR(device, swapChainKHR, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }
    void vkPresentImage(uint32_t& imageIndex) {
        VkPresentInfoKHR presentInfo
        { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChainKHR;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || VkWindow::framebufferResized) {
            VkWindow::framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
};