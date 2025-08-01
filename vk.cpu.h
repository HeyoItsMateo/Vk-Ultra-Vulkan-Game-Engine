#pragma once

#ifndef hCPU
#define hCPU

#include "vk.swapchain.h"
//TODO: Centralize power.

namespace vk {
    struct Fence {
        VkFence fence;
    public:
        static void signal(VkDevice device, VkFence fence) {
            vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &fence);
        }

        static void create(VkDevice device, VkFence fence, VkFenceCreateFlags flags = 0) {
            VkFenceCreateInfo createInfo
            { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            createInfo.flags = flags;

            vkCreateFence(device, &createInfo, nullptr, &fence);
        }

        static void destroy(VkDevice device, VkFence fence) {
            vkDestroyFence(device, fence, nullptr);
        } 
    };

    struct _Fence {
        template <int count>
        static void signal(const VkDevice device, const VkFence(&fences)[count]) {
            vkWaitForFences(device, count, fences, VK_TRUE, UINT64_MAX);
            vkResetFences(device, count, fences);
        }

        static void create(const VkDevice device, VkFence* const fence, VkFenceCreateFlags flags = 0)
        {
            VkFenceCreateInfo createInfo
            { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            createInfo.pNext = nullptr;
            createInfo.flags = flags;

            vkCreateFence(device, &createInfo, nullptr, fence);
        }

        static void destroy(const VkDevice device, VkFence* const fence) {
            vkDestroyFence(device, *fence, nullptr);
        }

        static void destroy(const VkDevice device, std::vector<VkFence> const& fences) {
            std::for_each(std::execution::par,
                fences.begin(), fences.end(),
                [&](auto const& fence)
                { vkDestroyFence(device, fence, nullptr); });
        }

        static void destroy(const VkDevice device, std::array<VkFence[2], MAX_FRAMES_IN_FLIGHT> renderFences) {
            std::for_each(std::execution::par,
                renderFences.begin(), renderFences.end(),
                [&](VkFence* const& fence)
                {
                    vkDestroyFence(device, fence[0], nullptr);
                    vkDestroyFence(device, fence[1], nullptr);
                });
        }
    };

    struct _Semaphore {
        static void destroy(const VkDevice device, VkSemaphore* const semaphores) {
            std::for_each(std::execution::par,
                semaphores, semaphores + MAX_FRAMES_IN_FLIGHT,
                [&](VkSemaphore const& semaphore)
                { vkDestroySemaphore(device, semaphore, nullptr); });
        }
        template<int count>
        static void destroy(const VkDevice device, VkSemaphore(&semaphores)[count]) {
            std::for_each(std::execution::par,
                semaphores, semaphores + count,
                [&](VkSemaphore const& semaphore)
                { vkDestroySemaphore(device, semaphore, nullptr); });
        }
        static void destroy(const VkDevice device, std::vector<VkSemaphore> const& semaphores) {
            std::for_each(std::execution::par,
                semaphores.begin(), semaphores.end(),
                [&](VkSemaphore const& semaphore)
                { vkDestroySemaphore(device, semaphore, nullptr); });
        }
    };

    template<int bufferCount = MAX_FRAMES_IN_FLIGHT>
    struct CPU_ {
        CPU_() {
            createCommandPool(pool);
            allocateCommandBuffers(pool, cmdBuffers, bufferCount);
        }
        ~CPU_() {
            vkFreeCommandBuffers(GPU::device, pool, bufferCount, cmdBuffers);
            vkDestroyCommandPool(GPU::device, pool, nullptr);
        }
    public:
        VkCommandPool pool;
        VkCommandBuffer cmdBuffers[bufferCount];
        void submitCommands(VkCommandBuffer* cmdBuffers, uint32_t bufferCount) {
            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = bufferCount;
            submitInfo.pCommandBuffers = cmdBuffers;

            VK_CHECK_RESULT(vkQueueSubmit(GPU::graphicsQueue, 1, &submitInfo, fence));
            signal(fence);

            vkResetCommandBuffer(*cmdBuffers, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        }
    protected:
        static void beginCommand(VkCommandBuffer& cmdBuffer) {
            VkCommandBufferBeginInfo beginInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
        }
        static void endCommand(VkCommandBuffer& cmdBuffer) {
            vkEndCommandBuffer(cmdBuffer);
        }
    private:
        
        static void allocateCommandBuffers(VkCommandPool& pool, VkCommandBuffer* cmdBuffers, uint32_t bufferCount) {
            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            VK_CHECK_RESULT(vkAllocateCommandBuffers(GPU::device, &allocInfo, cmdBuffers));
        }
    };

    struct CommandUnit : GPU_Object {
        CommandUnit(GPU* const pHost, const uint32_t queueFamilyIndex)
            : GPU_Object(pHost)
        {
            VkCommandPoolCreateInfo createInfo
            { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            createInfo.queueFamilyIndex = queueFamilyIndex;

            VK_CHECK_RESULT(vkCreateCommandPool(pHost->device, &createInfo, nullptr, &pool));
        }
        ~CommandUnit() {
            vkFreeCommandBuffers(pHost->device, pool, commands.size(), commands.data());
            vkDestroyCommandPool(pHost->device, pool, nullptr);
        }
    public:
        Fence fence;

        VkCommandBuffer* allocateBuffers(const uint16_t count) {
            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = count;

            VkCommandBuffer* commandBuffers = nullptr;
            VK_CHECK_RESULT(vkAllocateCommandBuffers(pHost->device, &allocInfo, commandBuffers));
            return commandBuffers;
        }

        void submit() {
            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = commands.size();
            submitInfo.pCommandBuffers = commands.data();

            VK_CHECK_RESULT(vkQueueSubmit(pHost->graphicsQueue, 1, &submitInfo, fence.fence));
            

            // TODO: figure out how to put all the reset calls into one section
            Fence::signal(pHost->device, fence.fence);
            reset();
        }
        void reset(const VkCommandPoolResetFlagBits flags  = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT) {
            vkResetCommandPool(pHost->device, pool, flags);
        }
    protected:
        std::vector<VkCommandBuffer> commands;
    private:
        VkCommandPool pool;

        friend struct RenderUnit;
    };

    struct RenderCommands {
        std::vector<VkCommandBuffer> buffers;

        std::vector<VkSemaphore> imageAvailable;
        std::vector<VkSemaphore> imageCompleted;
        std::vector<VkFence> inFlightFences;
    };

    struct Commands {
        
    public:
        Commands() {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
            submitInfo.signalSemaphoreCount = MAX_FRAMES_IN_FLIGHT;
        }
        void submit(const VkQueue queue, const VkFence fence) {
            VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
        }
    protected:
        VkSubmitInfo submitInfo;
    };

    struct ComputeCommands : Commands {
        ComputeCommands() {
            submitInfo.waitSemaphoreCount = MAX_FRAMES_IN_FLIGHT;
            submitInfo.pWaitSemaphores = finishedSemaphores;
            submitInfo.pCommandBuffers = buffers;
            submitInfo.pSignalSemaphores = finishedSemaphores;
        }
    public:
        static VkCommandBuffer* buffers;
        static VkSemaphore* finishedSemaphores;
    };

    struct GraphicsCommands : Commands {
        GraphicsCommands(VkSemaphore* const computeSemaphores)
        {
            waitSemaphores = new VkSemaphore[2 * MAX_FRAMES_IN_FLIGHT];
            std::copy(computeSemaphores, computeSemaphores + MAX_FRAMES_IN_FLIGHT, waitSemaphores);
            std::copy(readySemaphores, readySemaphores + MAX_FRAMES_IN_FLIGHT, waitSemaphores + MAX_FRAMES_IN_FLIGHT);

            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

            submitInfo.waitSemaphoreCount = 2 * MAX_FRAMES_IN_FLIGHT;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
        }
    public:
        
        static VkCommandBuffer* buffers;
        static VkSemaphore* readySemaphores;
        static VkSemaphore* finishedSemaphores;
    private:
        static VkSemaphore* waitSemaphores;
    };

    struct RenderUnit {
        // Constructor for if a command unit does not already exist, or the render unit needs it's own command unit.
        RenderUnit(GPU* const pHost, const uint32_t graphicsIndex)
        {
            cmdUnit = new CommandUnit(pHost, graphicsIndex);
            computeCommands = cmdUnit->allocateBuffers(MAX_FRAMES_IN_FLIGHT);
        }
        // Constructor for render unit if a command unit already exists.
        RenderUnit(CommandUnit* const cmdUnit)
        {
            this->cmdUnit = cmdUnit;
            computeCommands = cmdUnit->allocateBuffers(MAX_FRAMES_IN_FLIGHT);
        }
        // Destructor to clean up synchronization objects
        ~RenderUnit() {
            std::jthread tG0([&] { _Semaphore::destroy(cmdUnit->pHost->device, cmdGraphics->readySemaphores); });
            std::jthread tG1([&] { _Semaphore::destroy(cmdUnit->pHost->device, cmdGraphics->finishedSemaphores); });

            std::jthread tC0([&] { _Semaphore::destroy(cmdUnit->pHost->device, cmdCompute->finishedSemaphores); });

            std::jthread tR0([&] { _Fence::destroy(cmdUnit->pHost->device, renderFences); });
        }
    public:
        static ComputeCommands* cmdCompute;
        static GraphicsCommands* cmdGraphics;

        void sync(const uint32_t frameIndex) {
            _Fence::signal(cmdUnit->pHost->device, renderFences[frameIndex]);
            /*
            vkWaitForFences(cmdUnit->device, 2, inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
            vkResetFences(cmdUnit->device, 2, inFlightFences[frameIndex]);
            */
            // TODO: find out how to put all of the resets into one section
            cmdUnit->reset();
            //vkResetCommandBuffer(computeCommands[frameIndex], /*VkCommandBufferResetFlagBits*/ 0);
        }

        void submitCompute(const uint32_t frameIndex) {
            cmdCompute->submit(cmdUnit->pHost->computeQueue, renderFences[frameIndex][0]);
        }

        void submitGraphics(const uint32_t frameIndex) {
            cmdGraphics->submit(cmdUnit->pHost->graphicsQueue, renderFences[frameIndex][1]);
        }
    private:
        static CommandUnit* cmdUnit;
        static VkCommandBuffer* computeCommands;

        static std::array<VkFence[2], MAX_FRAMES_IN_FLIGHT> renderFences;
        // VkFence[0] -> cmpFence
        // VkFence[1] -> gfxFence
        
        //std::array<VkSemaphore[2], MAX_FRAMES_IN_FLIGHT> inFlightFences;
    };

    struct _EngineCPU {
        _EngineCPU(GPU* const pGPU) : device(pGPU->device)
        {
            createCommandPool(pGPU, pool);
            createSyncObjects(this);

            createCommandBuffers(renderCommands);
            createCommandBuffers(computeCommands);
        }
        ~_EngineCPU() {

            destroySyncObjects(this);
            vkDestroyCommandPool(device, pool, nullptr);
        }
    public:
        inline static std::vector<VkCommandBuffer> renderCommands;
        inline static std::vector<VkCommandBuffer> computeCommands;

        std::vector<VkSemaphore> imageAvailable;
        std::vector<VkSemaphore> imageCompleted;
        std::vector<VkFence> inFlightFences;

        std::vector<VkSemaphore> computeFinishedSemaphores;
        std::vector<VkFence> computeInFlightFences;

        void vkComputeSync(const uint32_t imageIndex) {
            vkWaitForFences(device, 1, &computeInFlightFences[imageIndex], VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &computeInFlightFences[imageIndex]);
            vkResetCommandBuffer(computeCommands[imageIndex], /*VkCommandBufferResetFlagBits*/ 0);
        }

        void vkRenderSync(const uint32_t imageIndex) {
            vkWaitForFences(device, 1, &inFlightFences[imageIndex], VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &inFlightFences[imageIndex]);
            vkResetCommandBuffer(renderCommands[imageIndex], /*VkCommandBufferResetFlagBits*/ 0);
        }

        void vkSubmitGraphicsQueue(GPU* const pGPU, const uint32_t imageIndex) {
            VkSemaphore waitSemaphores[] = { computeFinishedSemaphores[imageIndex], imageAvailable[imageIndex] };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.waitSemaphoreCount = 2 * MAX_FRAMES_IN_FLIGHT;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
            submitInfo.pCommandBuffers = renderCommands.data();
            submitInfo.signalSemaphoreCount = MAX_FRAMES_IN_FLIGHT;
            submitInfo.pSignalSemaphores = imageCompleted.data();

            VK_CHECK_RESULT(vkQueueSubmit(pGPU->graphicsQueue, 1, &submitInfo, inFlightFences[imageIndex]));
        }

        void vkSubmitComputeQueue(GPU* const pGPU, const uint32_t imageIndex) {
            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &computeCommands[imageIndex];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &computeFinishedSemaphores[imageIndex];

            VK_CHECK_RESULT(vkQueueSubmit(pGPU->computeQueue, 1, &submitInfo, computeInFlightFences[imageIndex]));
        }

        
    private:
        VkCommandPool pool;
        VkDevice device;

        static void createCommandPool(GPU* const pGPU, VkCommandPool& pool) {
            VkCommandPoolCreateInfo createInfo
            { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            createInfo.queueFamilyIndex = pGPU->graphicsFamily.value();

            VK_CHECK_RESULT(vkCreateCommandPool(pGPU->device, &createInfo, nullptr, &pool));
        }

        static void createSyncObjects(_EngineCPU* const pEngCPU) {
            VkDevice device = pEngCPU->device;
            // Initialize Graphics Synchronization objects
            std::jthread tG0([&] { 
                pEngCPU->imageAvailable.resize(MAX_FRAMES_IN_FLIGHT); 
                create_Semaphores(device, pEngCPU->imageAvailable);
                });
            std::jthread tG1([&] {
                pEngCPU->imageCompleted.resize(MAX_FRAMES_IN_FLIGHT);
                create_Semaphores(device, pEngCPU->imageCompleted);
                });
            std::jthread tG2([&] {
                pEngCPU->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
                create_Fences(device, pEngCPU->inFlightFences);
                });
            // Initialize Compute Synchronization objects
            std::thread tC0([&] {
                pEngCPU->computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
                create_Semaphores(device, pEngCPU->computeFinishedSemaphores);
                });
            std::thread tC1([&] {
                pEngCPU->computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
                create_Fences(device, pEngCPU->computeInFlightFences);
                });
        }
        static void create_Semaphores(VkDevice device, std::vector<VkSemaphore>& semaphores) {
            VkSemaphoreCreateInfo semaphoreInfo
            { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

            std::for_each(std::execution::par, semaphores.begin(), semaphores.end(),
                [&](VkSemaphore& semaphore)
                {
                    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
                });
        }
        static void create_Fences(VkDevice device, std::vector<VkFence>& fences) {
            VkFenceCreateInfo fenceInfo
            { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            std::for_each(std::execution::par,
                fences.begin(), fences.end(),
                [&](VkFence& fence)
                {
                    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
                });
        }

        static void destroySyncObjects(_EngineCPU* const pEngCPU) {
            std::jthread tG0([&] { destroy_Semaphores(pEngCPU->device, pEngCPU->imageCompleted); });
            std::jthread tG1([&] { destroy_Semaphores(pEngCPU->device, pEngCPU->imageAvailable); });
            std::jthread tG2([&] { destroy_Fences(pEngCPU->device, pEngCPU->inFlightFences); });

            std::jthread tC0([&] { destroy_Semaphores(pEngCPU->device, pEngCPU->computeFinishedSemaphores); });
            std::jthread tC1([&] { destroy_Fences(pEngCPU->device, pEngCPU->computeInFlightFences); });
        }
        static void destroy_Semaphores(VkDevice device, std::vector<VkSemaphore> const& semaphores) {
            std::for_each(std::execution::par,
                semaphores.begin(), semaphores.end(),
                [&](auto const& semaphore)
                { vkDestroySemaphore(device, semaphore, nullptr); });
        }
        static void destroy_Fences(VkDevice device, std::vector<VkFence> const& fences) {
            std::for_each(std::execution::par,
                fences.begin(), fences.end(),
                [&](auto const& fence)
                { vkDestroyFence(device, fence, nullptr); });
        }

        void createCommandBuffers(std::vector<VkCommandBuffer>& buffers) {
            buffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.commandPool = pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)buffers.size();

            VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, buffers.data()));
        }
    };
    
    struct Command : CPU_<>, Fence, GPU_Object {
        Command(GPU* const pHost)
            : GPU_Object(pHost)
        {
            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = 1;

            VK_CHECK_RESULT(vkAllocateCommandBuffers(pHost->device, &allocInfo, &cmdBuffer));
        }

        //TODO: Parallelize single time commands
        void beginCommand() {
            VkCommandBufferAllocateInfo allocInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = 1; //TODO: find out how to allocate two command buffers for parallel writing/submission

            VK_CHECK_RESULT(vkAllocateCommandBuffers(pHost->device, &allocInfo, &cmdBuffer));

            VkCommandBufferBeginInfo beginInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        }

        void endCommand(VkQueue queue) {
            vkEndCommandBuffer(cmdBuffer);

            VkSubmitInfo submitInfo
            { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1; //TODO: find out how to allocate two or more command buffers
            submitInfo.pCommandBuffers = &cmdBuffer;

            VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
            signal(pHost->device, fence);

            vkFreeCommandBuffers(pHost->device, pool, 1, &cmdBuffer);
        }
    protected:
        VkCommandBuffer cmdBuffer;
    };
    
}
#endif