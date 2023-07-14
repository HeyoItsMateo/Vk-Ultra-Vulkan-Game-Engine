#ifndef hVulkanCPU
#define hVulkanCPU

struct Fence {
    VkFence fence;
    Fence(VkFenceCreateFlags flags = 0) {
        VkFenceCreateInfo createInfo
        { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        createInfo.flags = flags;

        vkCreateFence(VkGPU::device, &createInfo, nullptr, &fence);
    }
    ~Fence() {
        vkDestroyFence(VkGPU::device, fence, nullptr);
    }
public:
    void signal() {
        vkWaitForFences(VkGPU::device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(VkGPU::device, 1, &fence);
    }
};

struct VkCPU {
    VkCommandPool pool;
    VkCPU() {
        VkCommandPoolCreateInfo createInfo
        { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = VkGPU::graphicsFamily.value();

        if (vkCreateCommandPool(VkGPU::device, &createInfo, nullptr, &pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }
    ~VkCPU() {
        vkDestroyCommandPool(VkGPU::device, pool, nullptr);
    }
};

struct VkCommand : VkCPU, Fence {
    VkCommand() {
        VkCommandBufferAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(VkGPU::device, &allocInfo, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
    //TODO: Parallelize single time commands
    void beginCommand() {
        VkCommandBufferAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = pool;
        allocInfo.commandBufferCount = 1; //TODO: find out how to allocate two command buffers for parallel writing/submission

        vkAllocateCommandBuffers(VkGPU::device, &allocInfo, &buffer);

        VkCommandBufferBeginInfo beginInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(buffer, &beginInfo);
    }
    void endCommand() {
        vkEndCommandBuffer(buffer);

        VkSubmitInfo submitInfo
        { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1; //TODO: find out how to allocate two or more command buffers
        submitInfo.pCommandBuffers = &buffer;

        vkQueueSubmit(VkGPU::graphicsQueue, 1, &submitInfo, fence);
        signal();

        vkFreeCommandBuffers(VkGPU::device, pool, 1, &buffer);
    }
protected:
    VkCommandBuffer buffer;
};

struct VkEngineSync {
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> computeInFlightFences;

    VkEngineSync() {
        init_Containers();

        std::jthread tG0([this] { create_Semaphores(imageAvailableSemaphores); });
        std::jthread tG1([this] { create_Semaphores(renderFinishedSemaphores); });
        std::jthread tG2([this] { create_Fences(inFlightFences); });

        std::jthread tC0([this] { create_Semaphores(computeFinishedSemaphores); });
        std::jthread tC1([this] { create_Fences(computeInFlightFences); });
    }
    ~VkEngineSync() {
        std::jthread tG0([this] { destroy_Semaphores(renderFinishedSemaphores); });
        std::jthread tG1([this] { destroy_Semaphores(imageAvailableSemaphores); });
        std::jthread tG2([this] { destroy_Fences(inFlightFences); });

        std::jthread tC0([this] { destroy_Semaphores(computeFinishedSemaphores); });
        std::jthread tC1([this] { destroy_Fences(computeInFlightFences); });
    }
private:
    void init_Containers() {
        std::jthread tG0([&] { imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT); });
        std::jthread tG1([&] { renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT); });
        std::jthread tG2([&] { inFlightFences.resize(MAX_FRAMES_IN_FLIGHT); });

        std::jthread tC0([&] { computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT); });
        std::jthread tC1([&] { computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT); });
    }

    void create_Semaphores(std::vector<VkSemaphore>& semaphores) {
        VkSemaphoreCreateInfo semaphoreInfo
        { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        std::for_each(std::execution::par, semaphores.begin(), semaphores.end(),
            [&](VkSemaphore& semaphore)
            {
                if (vkCreateSemaphore(VkGPU::device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create semaphore for a frame!");
                }
            });
    }
    void create_Fences(std::vector<VkFence>& fences) {
        VkFenceCreateInfo fenceInfo
        { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        std::for_each(std::execution::par,
            fences.begin(), fences.end(),
            [&](VkFence& fence)
            {
                if (vkCreateFence(VkGPU::device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create fence for a frame!");
                }
            });
    }

    void destroy_Semaphores(std::vector<VkSemaphore> const& semaphores) {
        std::for_each(std::execution::par,
            semaphores.begin(), semaphores.end(),
            [&](auto const& semaphore)
            { vkDestroySemaphore(VkGPU::device, semaphore, nullptr); });
    }
    void destroy_Fences(std::vector<VkFence> const& fences) {
        std::for_each(std::execution::par,
            fences.begin(), fences.end(),
            [&](auto const& fence)
            { vkDestroyFence(VkGPU::device, fence, nullptr); });
    }
};

struct VkEngineCPU : VkEngineSync, VkCPU {
    std::vector<VkCommandBuffer> renderCommands;
    std::vector<VkCommandBuffer> computeCommands;
    VkEngineCPU() {
        createCommandBuffers(renderCommands);
        createCommandBuffers(computeCommands);
    }
protected:
    void vkComputeSync() {
        vkWaitForFences(VkGPU::device, 1, &computeInFlightFences[VkSwapChain::currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(VkGPU::device, 1, &computeInFlightFences[VkSwapChain::currentFrame]);
        vkResetCommandBuffer(computeCommands[VkSwapChain::currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    }
    void vkRenderSync() {
        vkWaitForFences(VkGPU::device, 1, &inFlightFences[VkSwapChain::currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(VkGPU::device, 1, &inFlightFences[VkSwapChain::currentFrame]);
        vkResetCommandBuffer(renderCommands[VkSwapChain::currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    }
    void vkSubmitComputeQueue() {
        VkSubmitInfo submitInfo
        { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeCommands[VkSwapChain::currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &computeFinishedSemaphores[VkSwapChain::currentFrame];

        if (vkQueueSubmit(VkGPU::computeQueue, 1, &submitInfo, computeInFlightFences[VkSwapChain::currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer!");
        };
    }
    void vkSubmitGraphicsQueue() {
        VkSemaphore waitSemaphores[] = { computeFinishedSemaphores[VkSwapChain::currentFrame], imageAvailableSemaphores[VkSwapChain::currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo
        { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &renderCommands[VkSwapChain::currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[VkSwapChain::currentFrame];

        if (vkQueueSubmit(VkGPU::graphicsQueue, 1, &submitInfo, inFlightFences[VkSwapChain::currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }
private:
    void createCommandBuffers(std::vector<VkCommandBuffer>& buffers) {
        buffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)buffers.size();

        if (vkAllocateCommandBuffers(VkGPU::device, &allocInfo, buffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
};

#endif