#ifndef hFileSystem
#define hFileSystem

#define NOMINMAX
#define FILE_LOG "shaders/logs/updates.txt"

#include <Windows.h>
#include <processthreadsapi.h>
#include <filesystem>
#include <format>
#include <sstream>

#include <map>

static void runExe(const char* executable, char* commandline) {
    STARTUPINFOA sui = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(executable, commandline, NULL, NULL, true, 0, NULL, NULL, &sui, &pi)) {
        std::cout << GetLastError();
        abort();
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

struct VkFile {
    VkFile(std::string& fileName) {
        _path = std::filesystem::absolute(".\\shaders\\" + fileName).string();
        _time = std::format("{}", std::filesystem::last_write_time(_path));
    }
public:
    void update(std::string& last_update_time) {
        if (last_update_time != _time) {
            last_update_time = _time;
            std::string commandline = "-c " + _path + " -o " + _path + ".spv";
            runExe("C:\\VulkanSDK\\1.3.250.1\\Bin\\glslc.exe", commandline.data());

            std::cout << std::format("Shader update completed at {}\n", std::chrono::system_clock::now() + std::chrono::hours(5));
        }
    }
private:
    std::string _path;
    std::string _time;
};


static void compileShaderv2(const std::string& filename) {// TODO: Move compiler to the shader object for auto-updating
    if (!std::filesystem::exists(FILE_LOG)) {
        std::ofstream(FILE_LOG, std::ios::binary);
    }
    std::fstream log(FILE_LOG, std::ios::binary);
    if (!log.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}!", FILE_LOG));
    }

    std::pair<std::string, std::filesystem::file_time_type> loggedShader;
    while (!log.eof()) {
        log.read(reinterpret_cast<char*>(&loggedShader), sizeof(loggedShader));
        /*
        if ((loggedShader.first == shaderFile.first) and (loggedShader.second != shaderFile.second))
        {
            std::streampos infoLoc = log.tellg();
            log.seekg(infoLoc);
            log.write(reinterpret_cast<const char*>(&shaderFile), sizeof(shaderFile));

            std::string commandline = "-c " + shaderFile.first + " -o " + shaderFile.first + ".spv";
            runExe("C:\\VulkanSDK\\1.3.250.1\\Bin\\glslc.exe", commandline.data());

            std::cout << std::format("Shader update completed at {}\n", std::chrono::system_clock::now() + std::chrono::hours(5));
            break;
        }
        */
    }
    log.close();
}

static void compileShader(std::string filename) {// TODO: Move compiler to the shader object for auto-updating
    if (!std::filesystem::exists(FILE_LOG)) {
        std::ofstream(FILE_LOG);
    }
    std::fstream log;
    log.open(FILE_LOG);
    if (!log) {
        throw std::runtime_error("Failed to open log!");
    }
    VkFile shader(filename);
    
    std::string loggedTime;
    std::getline(log, loggedTime);
    log.seekg(0);
    shader.update(loggedTime);
    log << loggedTime << std::flush;
    log.close();
}

#endif