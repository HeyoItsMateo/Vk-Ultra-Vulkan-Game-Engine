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

        VkImageViewCreateInfo viewInfo
        { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            viewInfo.image = swapChainImages[i];
            if (vkCreateImageView(GPU::device, &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image view!");
            }
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
        VkAttachmentDescription colorAttachment = color.createAttachment();
        VkAttachmentDescription depthAttachment = depth.createAttachment();
        VkAttachmentDescription colorResolve    = color.createResolve();

        std::array<VkAttachmentDescription, 3> attachments 
        { colorAttachment, depthAttachment, colorResolve };

        VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        VkAttachmentReference depthAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        VkAttachmentReference colorResolveRef    = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        // Render Subpasses and Subpass Dependencies
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorResolveRef;

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

