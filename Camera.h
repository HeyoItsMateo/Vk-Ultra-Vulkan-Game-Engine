#pragma once
#ifndef hCamera
#define hCamera

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "vk.swapchain.h"

namespace vk {
    inline void gravity(glm::vec3& position, float& velocity) {
        double currentTime = glfwGetTime();
        double dt = (currentTime - SwapChain::lastTime);
        position.y += velocity * dt - 9.8 * 0.5 * dt * dt;
    }

    struct Camera {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec3 position;
        Camera();
        void update(float FOVdeg, float nearPlane, float farPlane);
    protected:
        glm::vec3 Orientation;
        glm::vec3 Up;
        float velocity = 0.05f;
        float sensitivity = 1.75f;
    private:
        bool firstClick = true;
        bool noClip = true;
        void keyboard_Input();
        void controller_Input();
    };
}

#endif