#include "VulkanGPL.h"

struct VkRPU {
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> commandBuffers2;
    std::vector<VkCommandBuffer> computeBuffers;
    VkRPU() {
        createCommandPool();
        createCommandBuffers(commandBuffers);
        createCommandBuffers(commandBuffers2);
        createCommandBuffers(computeBuffers);
    }
    ~VkRPU() {
        vkDestroyCommandPool(VkGPU::device, commandPool, nullptr);
    }
    //TODO: Parallelize single time commands
    void beginSingleTimeCommands(VkCommandBuffer& commandBuffer, uint32_t bufferCount = 1) {
        VkCommandBufferAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = bufferCount; //TODO: find out how to allocate two command buffers for parallel writing/submission

        vkAllocateCommandBuffers(VkGPU::device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
    }
    void endSingleTimeCommands(VkCommandBuffer& commandBuffer, uint32_t bufferCount = 1) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo
        { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = bufferCount; //TODO: find out how to allocate two or more command buffers
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(VkGPU::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VkGPU::graphicsQueue);

        vkFreeCommandBuffers(VkGPU::device, commandPool, 1, &commandBuffer);
    }
private:
    void createCommandPool() {
        VkCommandPoolCreateInfo poolInfo
        { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = VkGPU::graphicsFamily.value();

        if (vkCreateCommandPool(VkGPU::device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }
    void createCommandBuffers(std::vector<VkCommandBuffer>& buffers) {
        buffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)buffers.size();

        if (vkAllocateCommandBuffers(VkGPU::device, &allocInfo, buffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
};

struct VkSyncObjects {
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> computeInFlightFences;

    VkSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo
        { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        VkFenceCreateInfo fenceInfo
        { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(VkGPU::device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(VkGPU::device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(VkGPU::device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
            if (vkCreateSemaphore(VkGPU::device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(VkGPU::device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
    ~VkSyncObjects() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(VkGPU::device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(VkGPU::device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(VkGPU::device, inFlightFences[i], nullptr);
            
            vkDestroySemaphore(VkGPU::device, computeFinishedSemaphores[i], nullptr);
            vkDestroyFence(VkGPU::device, computeInFlightFences[i], nullptr);
        }
    }
};

struct VkGraphicsEngine : VkSwapChain, VkSyncObjects, VkRPU {
    VkGraphicsEngine(VkWindow& pWindow) : VkSwapChain(pWindow) {
    }
    void run(VkGraphicsPipeline<Vertex>& pipeline, VkTestPipeline<Particle>& ptclPpln, VkComputePipeline& computePpln, VkStorageBuffer& testo, UBO& uniforms, VkUniformBuffer<UBO>& ubo, uint32_t setCount, VkDescriptorSet* sets) {
        

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            std::jthread t1(glfwSetKeyCallback, window, userInput);

            uniforms.update(window, Extent);
            ubo.update(currentFrame, &uniforms);

            ////////////////////

            uint32_t imageIndex;

            vkAquireImage(imageIndex);

            /**/
            // Compute Queue
            vkComputeSync();
            //ptclPpln.bind(computeBuffers[currentFrame], setCount, &sets[currentFrame]);
            computePpln.computeCommand(computeBuffers[currentFrame], setCount, sets);
            vkSubmitComputeQueue();
            /**/
            // Render Queue
            vkRenderSync();
            beginRender(commandBuffers[currentFrame], swapChainFramebuffers[imageIndex]);
            //ptclPpln.renderCommand(commandBuffers[currentFrame], swapChainFramebuffers[imageIndex], ssbo.mSSBO[currentFrame], setCount, sets);
            //pipeline.renderCommand(commandBuffers[currentFrame], swapChainFramebuffers[imageIndex], setCount, sets);
            //ptclPpln.renderTest(commandBuffers[currentFrame], ssbo.mSSBO[currentFrame], setCount, sets);
            ptclPpln.renderTest(commandBuffers[currentFrame], testo.Buffer[currentFrame], setCount, sets);
            pipeline.renderTest(commandBuffers[currentFrame], setCount, sets);
            
            endRender(commandBuffers[currentFrame]);

            VkSemaphore waitSemaphores[] = { computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame]};
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            VkCommandBuffer commands[] = {commandBuffers2[currentFrame], commandBuffers[currentFrame]};

            vkSubmitGraphicsQueue<2>(waitSemaphores, waitStages);
            vkPresentImage(imageIndex);

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            //drawFrame(pipeline, setCount, sets);
        }
        vkDeviceWaitIdle(device);
    }
private:
    void vkComputeSync() {
        vkWaitForFences(device, 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &computeInFlightFences[currentFrame]);
        vkResetCommandBuffer(computeBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    }

    void vkSubmitComputeQueue() {
        VkSubmitInfo submitInfo
        { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

        if (vkQueueSubmit(computeQueue, 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer!");
        };
    }

    void vkRenderSync() { //TODO: rename to vkRenderImages()
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    }
    void beginRender(VkCommandBuffer& commandBuffer, VkFramebuffer& frameBuffer) {
        VkCommandBufferBeginInfo beginInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo = vkBeginRenderPass(frameBuffer, clearValues);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)VkSwapChain::Extent.width;
        viewport.height = (float)VkSwapChain::Extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = VkSwapChain::Extent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }
    void endRender(VkCommandBuffer& commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
    template<int waitCount>
    // Create operator overload for '<<' operator so that the following works
    // vkSubmitQueue << Graphics << waitFlags
    // vkSubmitCommand << commandCount(1) << Commands(commandBuffers)
    void vkSubmitGraphicsQueue(VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitStages) {
        VkSubmitInfo submitInfo
        { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = waitCount;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }
    VkRenderPassBeginInfo vkBeginRenderPass(VkFramebuffer& swapChainFramebuffer, std::array<VkClearValue, 2>& clearValues) {
        VkRenderPassBeginInfo renderPassInfo
        { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = VkSwapChain::renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = VkSwapChain::Extent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        return renderPassInfo;
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
    void vkPresentImage(uint32_t imageIndex) {
        VkPresentInfoKHR presentInfo
        { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChainKHR;
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