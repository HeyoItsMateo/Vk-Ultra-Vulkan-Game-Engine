#pragma once

#ifndef hInstance
#define hInstance

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cassert>

#include <stdexcept>

#include <vector>
#include <array>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


#define VK_CHECK_RESULT(vk_result) {\
	if (vk_result) {\
		std::cout << "Fatal : VkResult is \"" << errorString(vk_result) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(vk_result == VK_SUCCESS);\
	}\
}

std::string errorString(VkResult errorCode);

namespace vk {
    struct Window {
        Window(const char* windowName, uint32_t WIDTH = 800, uint32_t HEIGHT = 600);
        ~Window();
    public:
        inline static GLFWwindow* handle; //GLFW window handle
        inline static bool framebufferResized = false;
    private:
        static void framebufferResizeCallback(GLFWwindow* handle, int width, int height);
    };

    struct Instance {
        Instance();
        ~Instance();
    public:
        inline static VkInstance instance;
        inline static VkSurfaceKHR surface;
        VkDebugUtilsMessengerEXT debugMessenger;
    protected:
        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    private:
        void createInstance();
        void setupDebugMessenger();
        void createSurface();

        bool checkValidationLayerSupport();
        VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator);
        
        std::vector<const char*> getRequiredExtensions();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    };

}

struct Utilities {
    static VkWriteDescriptorSet writeDescriptor(uint32_t binding, VkDescriptorType descriptorType, VkDescriptorSet descriptorSet) {
        VkWriteDescriptorSet writeDescriptor
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeDescriptor.dstSet = descriptorSet;
        writeDescriptor.dstBinding = binding;
        writeDescriptor.dstArrayElement = 0;
        writeDescriptor.descriptorType = descriptorType;
        writeDescriptor.descriptorCount = 1;
        return writeDescriptor;
    }
    static VkPipelineVertexInputStateCreateInfo vkCreateVertexInput(VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) {
        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        return vertexInputInfo;
    }
    static VkPipelineRasterizationStateCreateInfo vkCreateRaster(VkPolygonMode drawType, VkCullModeFlags cullType = VK_CULL_MODE_BACK_BIT, VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE) {
        VkPipelineRasterizationStateCreateInfo rasterizer
        { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = drawType;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = cullType;
        rasterizer.frontFace = frontFace;
        rasterizer.depthBiasEnable = VK_FALSE;
        return rasterizer;
    }
    static VkPipelineColorBlendStateCreateInfo vkCreateColorBlend(VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkBool32 logicOpEnable, VkLogicOp logicOp = VK_LOGIC_OP_COPY) {
        VkPipelineColorBlendStateCreateInfo colorBlending
        { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.logicOpEnable = logicOpEnable;
        colorBlending.logicOp = logicOp;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;
        return colorBlending;
    }
    static VkPipelineDepthStencilStateCreateInfo vkCreateDepthStencil() {
        VkPipelineDepthStencilStateCreateInfo depthStencil
        { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;

        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional

        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional
        return depthStencil;
    }
};

#endif