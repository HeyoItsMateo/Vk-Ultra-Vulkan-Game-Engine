#include "vk.swapchain.h"

namespace vk {
    /* Swapchain */
    SwapChain::SwapChain()
    {
        createSwapChain(); // Cannot parallelize
        std::thread tImageViews([this] { createImageViews(); });

        std::thread tColorImage([this] { color.createResource(); });
        std::thread tDepthImage([this] { depth.createResource(); });

        std::thread tRenderPass([this] { createRenderPass(); });

        tImageViews.join(); tColorImage.join(); tDepthImage.join(); tRenderPass.join();
        createFramebuffers();
    }
    SwapChain::~SwapChain()
    {
        std::for_each(std::execution::par, framebuffers.begin(), framebuffers.end(),
            [&](const auto& framebuffer) { vkDestroyFramebuffer(device, framebuffer, nullptr); });

        std::jthread tRenderPass([this] { vkDestroyRenderPass(device, renderPass, nullptr); });

        std::for_each(std::execution::par, swapChainImageViews.begin(), swapChainImageViews.end(),
            [&](const auto& imageView) { vkDestroyImageView(device, imageView, nullptr); });

        vkDestroySwapchainKHR(device, swapChainKHR, nullptr);
    }
    //Public:
    void SwapChain::recreateSwapChain()
    {//TODO:
        // Maybe simplify to something like "dateSwap-chan~"
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(vk::Window::handle, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();

        std::thread tImageViews([this] { createImageViews(); });
        std::thread tColorImage([this] { color.createResource(); });
        std::thread tDepthImage([this] { depth.createResource(); });
        tImageViews.join(); tColorImage.join(); tDepthImage.join();

        createFramebuffers();
    }
    //Private:
    void SwapChain::createSwapChain()
    {
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo
        { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = Extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { graphicsFamily.value(), presentFamily.value() };

        if (graphicsFamily != presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChainKHR) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChainKHR, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChainKHR, &imageCount, swapChainImages.data());
    }
    void SwapChain::createImageViews()
    {// TODO: 
        // Resolve with "vk.image.h"
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            Image::createImageView(swapChainImages[i], swapChainImageViews[i], color.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }
    void SwapChain::createFramebuffers()
    {
        framebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments = {
            color.ImageView,
            depth.ImageView,
            swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo
            { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = Extent.width;
            framebufferInfo.height = Extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    void SwapChain::createRenderPass()
    {// TODO: Make attachments part of images
        // Renderpass Attachments
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = color.format;
        colorAttachment.samples = msaaSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depth.format;
        depthAttachment.samples = msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = color.format;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentReference depthAttachmentRef
        { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        VkAttachmentReference colorAttachmentResolveRef
        { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

        // Render Subpasses and Subpass Dependencies
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // Renderpass Creation
        VkRenderPassCreateInfo renderPassInfo
        { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void SwapChain::cleanupSwapChain()
    {
        std::for_each(std::execution::par, framebuffers.begin(), framebuffers.end(),
            [&](const auto& framebuffer) { vkDestroyFramebuffer(device, framebuffer, nullptr); });

        std::jthread tColorImage([this] { color.destroyResource(); });
        std::jthread tDepthImage([this] { depth.destroyResource(); });

        std::for_each(std::execution::par, swapChainImageViews.begin(), swapChainImageViews.end(),
            [&](const auto& imageView) { vkDestroyImageView(device, imageView, nullptr); });

        vkDestroySwapchainKHR(device, swapChainKHR, nullptr);
    }

}

