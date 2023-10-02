#pragma once

#ifndef hPrimitives
#define hPrimitives

#include <glm/glm.hpp>

struct Vertex {
    glm::vec4 position;
    glm::vec4 color;
    glm::vec2 texCoord;

    inline static VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    static VkVertexInputBindingDescription bindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> attributes() {
        std::vector<VkVertexInputAttributeDescription> Attributes{
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}, // Position
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) }, // Color
            { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, texCoord) }     // Texture Coordinate
        };
        return Attributes;
    }
    static VkPipelineVertexInputStateCreateInfo vertexInput() {
        static auto bindingDescription = bindings();
        static auto attributeDescriptions = attributes();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        return vertexInputInfo;
    }
    static VkPipelineInputAssemblyStateCreateInfo inputAssembly() {
        VkPipelineInputAssemblyStateCreateInfo inputAssembly
        { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return inputAssembly;
    }
};

struct Particle {
    glm::vec4 position;
    glm::vec4 color;
    //glm::vec2 texCoord;
    glm::vec4 velocity;

    const static VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    static VkVertexInputBindingDescription bindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> attributes() {
        std::vector<VkVertexInputAttributeDescription> Attributes{
            { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, position)}, // Position
            { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, color) }  // Color
            //{ 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Particle, texCoord) }     // Texture Coordinate
        };
        return Attributes;
    }
    static VkPipelineVertexInputStateCreateInfo vertexInput() {
        static auto bindingDescription = bindings();
        static auto attributeDescriptions = attributes();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        return vertexInputInfo;
    }
    static VkPipelineInputAssemblyStateCreateInfo inputAssembly() {
        VkPipelineInputAssemblyStateCreateInfo inputAssembly
        { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return inputAssembly;
    }
};

template<VkPrimitiveTopology topology>
struct Primitive {
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec4 color;
    glm::vec2 texCoord;
    
    static VkVertexInputBindingDescription bindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Primitive);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }
    static std::vector<VkVertexInputAttributeDescription> attributes() {
        std::vector<VkVertexInputAttributeDescription> Attributes{
            { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Primitive, position) }, // Position
            { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Primitive, normal) },   // Normal
            { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Primitive, color) },   // Color
            { 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Primitive, texCoord) }      // Texture Coordinate
        };
        return Attributes;
    }
    static VkPipelineVertexInputStateCreateInfo vertexInput() {
        static auto bindingDescription = bindings();
        static auto attributeDescriptions = attributes();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        return vertexInputInfo;
    }
    static VkPipelineInputAssemblyStateCreateInfo inputAssembly() {
        VkPipelineInputAssemblyStateCreateInfo inputAssembly
        { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return inputAssembly;
    }
};

typedef Primitive<VK_PRIMITIVE_TOPOLOGY_LINE_LIST> lineList;
typedef Primitive<VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST> triangleList;

#endif