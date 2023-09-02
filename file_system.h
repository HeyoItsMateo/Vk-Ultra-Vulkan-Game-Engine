#ifndef hFileSystem
#define hFileSystem

#define NOMINMAX
#define SHADER_LOG "bin\\shader_log.bin"

#include <filesystem>
#include <format>
#include <sstream>

struct SPIRV {
    SPIRV(const std::string& filename) {
        std::string _path = std::filesystem::absolute(".\\shaders\\" + filename).string();
        last_write = std::format("{}", std::filesystem::last_write_time(filename));

        if (!std::filesystem::exists(SHADER_LOG)) {
            createLog(filename);
            compileGLSL(filename);
            return;
        }
        updateLog(filename);
    }
    ~SPIRV() {}
protected:
    std::string last_write;
    void createLog(const std::string& filename) {
        std::ofstream log(SHADER_LOG, std::ios_base::out | std::ios::binary);
        if (!log.is_open()) {
            throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
        }
        log << filename << '\0';
        log << last_write << '\0';
        log.close();
    }
    void updateLog(const std::string& filename) {
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
                    log.seekg(loc);
                    log << last_write << '\0';
                    compileGLSL(filename);
                }
                std::cout << std::format("{} is up to date.\n", filename);
                log.close();
                return;
            }
            std::getline(log, line, '\0');
        }
        log.close();

        appendLog(filename);
        compileGLSL(filename);
    }
    void appendLog(const std::string& filename) {
        std::ofstream log(SHADER_LOG, std::ios::out | std::ios::binary | std::ios::app);
        if (!log.is_open()) {
            throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
        }
        log << filename << '\0';
        log << last_write << '\0';
        log.close();
    }
private:
    void compileGLSL(const std::string& filename) {
        std::ofstream file("compiler.bat");
        if (!file.is_open()) {
            throw std::runtime_error("Runtime Error: Failed to create batch file!");
        }
        file << "@echo off\n";
        file << "C:\\VulkanSDK\\1.3.250.1\\Bin\\glslc.exe ";
        file << ".\\shaders\\" << filename;
        file << " -o .\\shaders\\" << filename << ".spv" << std::endl;
        file.close();

        std::system("compiler.bat");
        std::cout << std::format("Shader update completed at {}\n", std::chrono::system_clock::now() + std::chrono::hours(5));
    }
};

static void compileGLSL(const std::string& filename) {
    std::ofstream file("compiler.bat");
    if (!file)
    {
        throw std::runtime_error("Runtime Error: Failed to create batch file!");
    }

    std::filesystem::path filepath(filename);

    file << "@echo off\n";
    file << "C:\\VulkanSDK\\1.3.250.1\\Bin\\glslc.exe ";
    file << ".\\shaders\\" << filepath.string();
    file << " -o .\\shaders\\" << filepath.string() << ".spv" << std::endl;
    file.close();

    std::system("compiler.bat");
    std::cout << "Shader update completed at " << std::chrono::system_clock::now() + std::chrono::hours(5) << std::endl;
}

static void checkLog(const std::string& filename) {
    std::string _path = std::filesystem::absolute(".\\shaders\\" + filename).string();
    std::string last_write = std::format("{}", std::filesystem::last_write_time(_path));

    //File shaderFile(filename);
    if (!std::filesystem::exists(SHADER_LOG)) {
        std::ofstream out(SHADER_LOG, std::ios_base::out | std::ios::binary);
        if (!out.is_open()) {
            throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
        }
        compileGLSL(filename);
        out << filename << '\0';
        out << last_write << '\0';
        out.close();
        return;
    }
    std::fstream out(SHADER_LOG, std::ios_base::out | std::ios_base::in | std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
    }
    out.seekg(std::ios::beg);
    for (std::string line; std::getline(out, line, '\0');) {
        if (line == filename) {
            int loc = static_cast<int>(out.tellg());
            std::getline(out, line, '\0');
            if (line != last_write) {
                std::cout << std::format("Updating log entry for {}\n", filename);
                compileGLSL(filename);
                out.seekg(loc);
                out << last_write << '\0';
            }
            std::cout << std::format("{} is up to date.\n", filename);
            out.close();
            return;
        }
        std::getline(out, line, '\0');
    }
    out.close();

    compileGLSL(filename);
    std::ofstream update(SHADER_LOG, std::ios::out | std::ios::binary |std::ios::app);
    if (!update.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}!", SHADER_LOG));
    }
    update << filename << '\0';
    update << last_write << '\0';
    update.close();
}


#endif