#include "vk.swapchain.h"


namespace vk {
    /* Swapchain */
    Swapchain::Swapchain(GPU* const pGPU)
    {
        Swapchain::pGPU = pGPU;

        create(this); // Cannot parallelize
        std::thread tImageViews([this] { createImageViews(this); });

        std::thread tColorImage([this] { color.createResource(this->pGPU); });
        std::thread tDepthImage([this] { depth.createResource(this->pGPU); });

        std::thread tRenderPass([&] { createRenderPass(pGPU->device); });

        tImageViews.join(); tColorImage.join(); tDepthImage.join(); tRenderPass.join();
        createFramebuffers(this);
    }
    Swapchain::~Swapchain()
    {
        std::for_each(std::execution::par, framebuffers.begin(), framebuffers.end(),
            [&](const auto& framebuffer) { vkDestroyFramebuffer(pGPU->device, framebuffer, nullptr); });

        std::jthread tRenderPass([this] { vkDestroyRenderPass(pGPU->device, renderPass, nullptr); });

        std::for_each(std::execution::par, ImageViews.begin(), ImageViews.end(),
            [&](const auto& imageView) { vkDestroyImageView(pGPU->device, imageView, nullptr); });

        vkDestroySwapchainKHR(pGPU->device, KHR, nullptr);
    }
    //Public:
    void Swapchain::recreate(Swapchain* const pSwapchain)
    {
        GPU* const pGPU = pSwapchain->pGPU;
        const VkDevice device = pGPU->device;

        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(pGPU->pInstance->pWindow->handle, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device);

        cleanup(pSwapchain);

        create(pSwapchain);

        
        std::thread tImageViews([&] { createImageViews(pSwapchain); });
        std::thread tColorImage([&] { color.createResource(pGPU); });
        std::thread tDepthImage([&] { depth.createResource(pGPU); });
        tImageViews.join(); tColorImage.join(); tDepthImage.join();

        createFramebuffers(pSwapchain);
    }
    //Private:
    void Swapchain::create(Swapchain* const pSwapchain)
    {
        const GPU* pGPU = pSwapchain->pGPU;
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(pGPU->formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(pGPU->presentModes);

        uint32_t imageCount = pGPU->capabilities.minImageCount + 1;
        if (pSwapchain->pGPU->capabilities.maxImageCount > 0 && imageCount > pGPU->capabilities.maxImageCount) {
            imageCount = pGPU->capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo
        { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = pGPU->pInstance->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = pGPU->extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { pGPU->graphicsFamily.value(), pGPU->presentFamily.value() };

        if (pGPU->graphicsFamily != pGPU->presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = pGPU->capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        VK_CHECK_RESULT(vkCreateSwapchainKHR(pGPU->device, &createInfo, nullptr, &KHR));

        vkGetSwapchainImagesKHR(pGPU->device, KHR, &imageCount, nullptr);
        pSwapchain->images.resize(imageCount);
        vkGetSwapchainImagesKHR(pGPU->device, KHR, &imageCount, pSwapchain->images.data());
    }

    void Swapchain::createImageViews(Swapchain* const pSwapchain)
    {// TODO: 
        // Resolve with "vk.image.h"
        uint32_t imageCount = pSwapchain->images.size();

        pSwapchain->ImageViews.resize(imageCount);

        VkImageViewCreateInfo viewInfo
        { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        const VkDevice device = pSwapchain->pGPU->device;
        for (size_t i = 0; i < imageCount; i++) {
            viewInfo.image = pSwapchain->images[i];
            VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &pSwapchain->ImageViews[i]));
        }
    }

    void Swapchain::createFramebuffers(Swapchain* const pSwapchain)
    {
        
        GPU* const pGPU = pSwapchain->pGPU;
        const VkDevice device   = pGPU->device;
        const VkExtent2D extent = pGPU->extent;
        const uint32_t ImageViewCount = pSwapchain->ImageViews.size();
        pSwapchain->framebuffers.resize(ImageViewCount);

        for (size_t i = 0; i < ImageViewCount; i++) {
            std::array<VkImageView, 3> attachments = {
            color.view,
            depth.view,
            pSwapchain->ImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo
            { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &pSwapchain->framebuffers[i]));
        }
    }

    void Swapchain::createRenderPass(const VkDevice device)
    {
        VkAttachmentDescription colorAttachment = color.createAttachment(this->pGPU->msaaSamples);
        VkAttachmentDescription depthAttachment = depth.createAttachment(this->pGPU->msaaSamples);
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

        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    }

    void Swapchain::cleanup(Swapchain* const pSwapchain)
    {
        const VkDevice device = pSwapchain->pGPU->device;

        std::for_each(std::execution::par, pSwapchain->framebuffers.begin(), pSwapchain->framebuffers.end(),
            [&](const auto& framebuffer) { vkDestroyFramebuffer(device, framebuffer, nullptr); });

        std::jthread tColorImage([&] { color.destroyResource(device); });
        std::jthread tDepthImage([&] { depth.destroyResource(device); });

        std::for_each(std::execution::par, pSwapchain->ImageViews.begin(), pSwapchain->ImageViews.end(),
            [&](const auto& imageView) { vkDestroyImageView(device, imageView, nullptr); });

        vkDestroySwapchainKHR(device, pSwapchain->KHR, nullptr);
    }

}

