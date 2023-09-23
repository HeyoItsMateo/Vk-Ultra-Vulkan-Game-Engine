#include "vk.shader.h"

namespace vk {
    Shader::Shader(std::string const& filename, VkShaderStageFlagBits stage) : shaderStage(stage) {
        checkLog(filename);
        auto code = readFile(".\\shaders\\" + filename + ".spv");
        createModule(code, shaderModule);
    }
    Shader::~Shader() {
        if (shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(GPU::device, shaderModule, nullptr);
        }
    }
    /* Private */
    void Shader::createModule(const std::vector<char>& code, VkShaderModule& shaderModule) {
        VkShaderModuleCreateInfo createInfo
        { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VK_CHECK_RESULT(vkCreateShaderModule(GPU::device, &createInfo, nullptr, &shaderModule));
    }

    std::vector<char> Shader::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error(std::format("failed to open {}!", filename));
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}

