#ifndef hUBO
#define hUBO

#include "Camera.h"

#include "vk.buffers.h"
#include "descriptors.h"

#include <mutex>
#include <utility>

namespace vk {
    struct Uniforms {
        double dt = 1.0;
        alignas(16) glm::mat4 model = glm::mat4(1.f);
        Camera camera;
        void update(float FOVdeg = 45.f, float nearPlane = 0.01f, float farPlane = 1000.f) {
            std::jthread t1([&] { modelUpdate(); });
            std::jthread t2([&] { camera.update(FOVdeg, nearPlane, farPlane); });
            std::jthread t3([&] { deltaTime(); });
        }
    private:
        void modelUpdate() {
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        void deltaTime() {
            double currentTime = glfwGetTime();
            dt = (currentTime - SwapChain::lastTime);
        }
    }; 

    struct UBO : DataBuffer, Descriptor {
        template<typename T>
        inline UBO(T& uniforms, VkShaderStageFlags flag, uint32_t bindingCount = 1);
    public:
        template <typename T>
        inline void update(T& uniforms);
    private:
        StageBuffer_ stageUBO;
        void writeDescriptorSets(uint32_t bindingCount) override;
    };
}

#include "vk.ubo.ipp"

#endif