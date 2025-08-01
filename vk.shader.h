#pragma once

#include "vk.gpu.h"
#include <string>

namespace vk {
    struct Shader {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits shaderStage;
        Shader(std::string const& filename, VkShaderStageFlagBits stage);
        ~Shader();
    private:
        static std::vector<char> readFile(const std::string& filename);
        static void createModule(const std::vector<char>& code, VkShaderModule& shaderModule);
    };
    struct Shader_ {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits shaderStage;
        Shader_(std::string const& filename, VkShaderStageFlagBits stage);
        ~Shader_();
    private:
        static std::vector<char> readFile(const std::string& filename);
        static void createModule(const std::vector<char>& code, VkShaderModule& shaderModule);
    };
}

#ifndef LOG_FILE
#define LOG_FILE(log) "bin\\"#log"_log.bin"

#define SHADER_LOG LOG_FILE(shader)
#endif

#define NOMINMAX

#include <filesystem>
#include <format>
#include <chrono>

static void compileGLSL(const std::string& filename) {
    std::ofstream file("compiler.bat");
    if (!file) {
        throw std::runtime_error("Runtime Error: Failed to create batch file!");
    }

    std::filesystem::path filepath(filename);

    file << "@echo off\n";
    file << "C:\\VulkanSDK\\1.3.261.1\\Bin\\glslc.exe ";
    file << ".\\shaders\\glsl\\" << filepath.string();
    file << " -o .\\shaders\\" << filepath.string() << ".spv" << std::endl;
    file.close();

    std::system("compiler.bat");
    std::cout << "Shader update completed at " << std::chrono::system_clock::now() + std::chrono::hours(5) << std::endl;
}

static void checkLog(const std::string& filename) {
    std::string _path = std::filesystem::absolute(".\\shaders\\glsl\\" + filename).string();
    std::string last_write = std::format("{}", std::filesystem::last_write_time(_path));

    //File shaderFile(filename);
    if (!std::filesystem::exists(SHADER_LOG)) {
        std::ofstream log(SHADER_LOG, std::ios_base::out | std::ios::binary);
        if (!log.is_open()) {
            throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
        }
        compileGLSL(filename);
        log << filename << '\0';
        log << last_write << '\0';
        log.close();
        return;
    }
    if (!std::filesystem::exists(".\\shaders\\" + filename + ".spv")) {
        compileGLSL(filename);
        return;
    }

    std::fstream log(SHADER_LOG, std::ios_base::out | std::ios_base::in | std::ios::binary);
    if (!log.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
    }
    log.seekg(std::ios::beg);
    for (std::string line; std::getline(log, line, '\0');) {
        if (line == filename) {
            int loc = static_cast<int>(log.tellg());
            std::getline(log, line, '\0');
            if (line != last_write) {
                std::cout << std::format("Updating log entry for {}\n", filename);
                compileGLSL(filename);
                log.seekg(loc);
                log << last_write << '\0';
            }
            std::cout << std::format("{} is up to date.\n", filename);
            log.close();
            return;
        }
        // Consume last_write for other shaderfiles
        std::getline(log, line, '\0');
    }
    log.close();
    // SHADER_LOG and SPIR-V files exist, but a log entry for the shader does not exist.
    compileGLSL(filename);
    std::ofstream update(SHADER_LOG, std::ios::out | std::ios::binary | std::ios::app);
    if (!update.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
    }
    update << filename << '\0';
    update << last_write << '\0';
    update.close();
}