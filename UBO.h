#ifndef hUBO
#define hUBO

namespace vk {
    struct Uniforms {
        float dt = 1.f;
        alignas(16) glm::mat4 model = glm::mat4(1.f);
        Camera camera;
        void update(double lastTime, float FOVdeg = 45.f, float nearPlane = 0.01f, float farPlane = 1000.f) {
            std::jthread t1([this]
                { modelUpdate(); }
            );
            std::jthread t2([this, FOVdeg, nearPlane, farPlane]
                { camera.update(FOVdeg, nearPlane, farPlane); }
            );
            std::jthread t3([this, lastTime]
                { deltaTime(lastTime); }
            );
        }
    private:
        void modelUpdate() {
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        void deltaTime(double lastTime) {
            double currentTime = glfwGetTime();
            dt = (currentTime - lastTime);
        }
    };
}

#endif