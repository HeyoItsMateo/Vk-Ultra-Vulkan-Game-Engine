#include "VulkanGPL.h"

struct VkGraphicsEngine : VkSwapChain, VkEngineCPU {
    PhxModel model;
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

    
    VkGraphicsEngine() : model(vertices,indices) {

    }
    void run(VkGraphicsPipeline<Vertex>& pipeline, VkTestPipeline<Particle>& ptclPpln, VkComputePipeline& computePpln, VkStorageBuffer& ssbo, UBO& uniforms, VkUniformBuffer<UBO>& ubo, uint32_t setCount, VkDescriptorSet* sets) {
        while (!glfwWindowShouldClose(VkWindow::window)) {
            glfwPollEvents();
            std::jthread t1(glfwSetKeyCallback, VkWindow::window, userInput);
            std::jthread t2(&VkGraphicsEngine::deltaTime, this);

            updateSystem(uniforms, ubo);

            uint32_t imageIndex;
            vkAquireImage(imageIndex);

            // Compute Queue
            vkComputeSync();
            computePpln.computeCommand(computeCommands[currentFrame], setCount, sets);
            vkSubmitComputeQueue();

            // Render Queue
            vkRenderSync();
            beginRender(imageIndex);
            
            ptclPpln.bind(renderCommands[currentFrame], setCount, sets);
            ptclPpln.draw(renderCommands[currentFrame], ssbo.Buffer[currentFrame]);

            pipeline.bind(renderCommands[currentFrame], setCount, sets);
            model.draw(renderCommands[currentFrame]);
            
            endRender();

            vkSubmitGraphicsQueue();
            vkPresentImage(imageIndex);

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }
        vkDeviceWaitIdle(device);
    }
protected:
    double lastTime = 0.0;
    void deltaTime() {
        double currentTime = glfwGetTime();
        lastTime = currentTime;
    }
    void updateSystem(UBO& uniforms, VkUniformBuffer<UBO>& ubo) {
        uniforms.update(lastTime);
        ubo.update(currentFrame, &uniforms);

        updateVtx();
        model.update(testVtx);
    }
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