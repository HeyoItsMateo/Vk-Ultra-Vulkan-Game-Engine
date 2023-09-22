#ifndef hVulkanCPU
#define hVulkanCPU

#include "vk.swapchain.h"

namespace vk {
    struct Fence {
        VkFence fence;
        Fence(VkFenceCreateFlags flags = 0) {
            VkFenceCreateInfo createInfo
            { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            createInfo.flags = flags;

            vkCreateFence(GPU::device, &createInfo, nullptr, &fence);
        }
        ~Fence() {
            vkDestroyFence(GPU::device, fence, nullptr);
        }
    public:
        void signal() {
            vkWaitForFences(GPU::device, 1, &fence, VK_TRUE, UINT64_MAX);
            vkResetFences(GPU::device, 1, &fence);
        }
    };

    struct CPU_: Fence {
        //template<int size>
        //static void submitCommands(VkCommandBuffer(&pCommands)[size]) {
        //    VkSubmitInfo submitInfo
        //    { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        //    submitInfo.commandBufferCount = size; //TODO: find out how to allocate two or more command buffers
        //    submitInfo.pCommandBuffers = pCommands;

        //    vkQueueSubmit(GPU::graphicsQueue, 1, &submitInfo, fence);
        //    signal();

        //    //vkFreeCommandBuffers(GPU::device, pool, 1, pCommands);
        //}
    };

    struct CPU {
        VkCommandPool pool;
        CPU() {
            VkCommandPoolCreateInfo createInfo
            { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            createInfo.queueFamilyIndex = GPU::graphicsFamily.value();

            if (vkCreateCommandPool(GPU::device, &createInfo, nullptr, &pool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics command pool!");
            }
        }
        ~CPU() {
            vkDestroyCommandPool(GPU::device, pool, nullptr);
        }
    };

    struct Command : CPU, Fence {
        Command() {
            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = 1;

            VK_CHECK_RESULT(vkAllocateCommandBuffers(GPU::device, &allocInfo, &cmdBuffer));
        }
        //TODO: Parallelize single time commands
        void beginCommand() {
            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = 1; //TODO: find out how to allocate two command buffers for parallel writing/submission

            VK_CHECK_RESULT(vkAllocateCommandBuffers(GPU::device, &allocInfo, &cmdBuffer));

            VkCommandBufferBeginInfo beginInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
        }
        void endCommand() {
            vkEndCommandBuffer(cmdBuffer);

            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1; //TODO: find out how to allocate two or more command buffers
            submitInfo.pCommandBuffers = &cmdBuffer;

            vkQueueSubmit(GPU::graphicsQueue, 1, &submitInfo, fence);
            signal();

            vkFreeCommandBuffers(GPU::device, pool, 1, &cmdBuffer);
        }
    protected:
        VkCommandBuffer cmdBuffer;
    };

    struct EngineSync {
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        std::vector<VkSemaphore> computeFinishedSemaphores;
        std::vector<VkFence> computeInFlightFences;

        EngineSync() {
            init_Containers();

            std::jthread tG0([this] { create_Semaphores(imageAvailableSemaphores); });
            std::jthread tG1([this] { create_Semaphores(renderFinishedSemaphores); });
            std::jthread tG2([this] { create_Fences(inFlightFences); });

            std::jthread tC0([this] { create_Semaphores(computeFinishedSemaphores); });
            std::jthread tC1([this] { create_Fences(computeInFlightFences); });
        }
        ~EngineSync() {
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
                    if (vkCreateSemaphore(GPU::device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
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
                    if (vkCreateFence(GPU::device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
                    {
                        throw std::runtime_error("failed to create fence for a frame!");
                    }
                });
        }

        void destroy_Semaphores(std::vector<VkSemaphore> const& semaphores) {
            std::for_each(std::execution::par,
                semaphores.begin(), semaphores.end(),
                [&](auto const& semaphore)
                { vkDestroySemaphore(GPU::device, semaphore, nullptr); });
        }
        void destroy_Fences(std::vector<VkFence> const& fences) {
            std::for_each(std::execution::par,
                fences.begin(), fences.end(),
                [&](auto const& fence)
                { vkDestroyFence(GPU::device, fence, nullptr); });
        }
    };

    struct EngineCPU : EngineSync, CPU {
        inline static std::vector<VkCommandBuffer> renderCommands;
        inline static std::vector<VkCommandBuffer> computeCommands;
        EngineCPU() {
            createCommandBuffers(renderCommands);
            createCommandBuffers(computeCommands);
        }
    protected:
        void vkComputeSync() {
            vkWaitForFences(GPU::device, 1, &computeInFlightFences[SwapChain::currentFrame], VK_TRUE, UINT64_MAX);
            vkResetFences(GPU::device, 1, &computeInFlightFences[SwapChain::currentFrame]);
            vkResetCommandBuffer(computeCommands[SwapChain::currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        }
        void vkRenderSync() {
            vkWaitForFences(GPU::device, 1, &inFlightFences[SwapChain::currentFrame], VK_TRUE, UINT64_MAX);
            vkResetFences(GPU::device, 1, &inFlightFences[SwapChain::currentFrame]);
            vkResetCommandBuffer(renderCommands[SwapChain::currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        }
        void vkSubmitComputeQueue() {
            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &computeCommands[SwapChain::currentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &computeFinishedSemaphores[SwapChain::currentFrame];

            VK_CHECK_RESULT(vkQueueSubmit(GPU::computeQueue, 1, &submitInfo, computeInFlightFences[SwapChain::currentFrame]));
        }
        void vkSubmitGraphicsQueue() {
            VkSemaphore waitSemaphores[] = { computeFinishedSemaphores[SwapChain::currentFrame], imageAvailableSemaphores[SwapChain::currentFrame] };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.waitSemaphoreCount = 2;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &renderCommands[SwapChain::currentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphores[SwapChain::currentFrame];

            VK_CHECK_RESULT(vkQueueSubmit(GPU::graphicsQueue, 1, &submitInfo, inFlightFences[SwapChain::currentFrame]));
        }
    private:
        void createCommandBuffers(std::vector<VkCommandBuffer>& buffers) {
            buffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.commandPool = pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)buffers.size();

            VK_CHECK_RESULT(vkAllocateCommandBuffers(GPU::device, &allocInfo, buffers.data()));
        }
    };
}


#endif