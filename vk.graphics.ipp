namespace vk {
    template<typename primitiveType, VkPolygonMode polygonMode>
    template<uint32_t size>
    GraphicsPPL<primitiveType, polygonMode>::GraphicsPPL(Shader(&shaders)[size], std::vector<VkDescriptorSet> const& descSets, std::vector<VkDescriptorSetLayout>& SetLayout)
    {
        bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sets = descSets;
        vkLoadSetLayout(SetLayout, layout);
        vkCreatePipeline(stageInfo(shaders, size));
    }
    /* Private */
    template<typename primitiveType, VkPolygonMode polygonMode>
    std::vector<VkPipelineShaderStageCreateInfo> GraphicsPPL<primitiveType, polygonMode>::stageInfo(Shader* shaders, uint32_t size)
    {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pName = "main";

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages{ size, stageInfo };
        for (int i = 0; i < size; i++) {
            shader_stages[i].module = shaders[i].shaderModule;
            shader_stages[i].stage = shaders[i].shaderStage;
        }

        return shader_stages;
    }
    template<typename primitiveType, VkPolygonMode polygonMode>
    void GraphicsPPL<primitiveType, polygonMode>::vkCreatePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderStages)
    {

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = primitiveType::vertexInput();
        VkPipelineInputAssemblyStateCreateInfo vertexAssemblyInfo = primitiveType::inputAssembly();

        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateInfo = dynamicState(dynamicStates);
        VkPipelineViewportStateCreateInfo viewportInfo = viewportState(1, 1);

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterInfo = rasterState(polygonMode);
        VkPipelineMultisampleStateCreateInfo msaaInfo = msaaState(VK_TRUE, 0.2f);
        VkPipelineColorBlendStateCreateInfo colorBlendInfo = colorBlendState(colorBlendAttachment, VK_FALSE);
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = depthStencilState();

        VkGraphicsPipelineCreateInfo pipelineInfo
        { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.layout = layout;

        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();

        pipelineInfo.renderPass = SwapChain::renderPass;
        pipelineInfo.subpass = 0;

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &vertexAssemblyInfo;

        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.pViewportState = &viewportInfo;

        pipelineInfo.pRasterizationState = &rasterInfo;
        pipelineInfo.pMultisampleState = &msaaInfo;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
    }
}