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

#include <map>
typedef void(*glfwFunc)();

inline void close() {
    glfwSetWindowShouldClose(vk::Window::handle, true);
}
inline void pause() {
    vk::time = !vk::time;
}
inline std::map<int, void(*)()> keyboard_map
{
    { GLFW_KEY_ESCAPE, close },
    { GLFW_KEY_T, pause }
};
inline void userInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{// Sets Keyboard Commands
    //TODO: map
    if (action && keyboard_map[key]) {
        glfwFunc func = keyboard_map[key];
        (*func)();
    }
}

namespace vk {
    inline void gravity(glm::vec3& position, float& velocity) {
        position.y += (velocity * dt) + (-9.8 * (0.5 * (dt * dt)));
    }

    struct Camera {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec3 position;
        Camera();
        
    private:
        void Controller_Input();
        void Mouse_Input();
        void Keyboard_Input();
    public:
        void update(float FOVdeg, float nearPlane, float farPlane);
    protected:
        bool noClip = true;
        bool firstClick = true;

        float velocity = 0.05f;
        float sensitivity = 0.075f;
        double RS_deadzone_X = 0.1;
        double RS_deadzone_Y = 0.15;

        glm::vec3 Orientation;
        glm::vec3 Up;

        glm::quat test_Orientation;
    private:
        float rotX = 0.f;
        float rotY = 0.f;
        glm::quat qPitch = glm::angleAxis(0.f, glm::vec3(0, 1, 0));
        glm::quat qYaw = glm::angleAxis(0.f, glm::vec3(1, 0, 0));

        

        float rotation(const float input);
    };
}

#endif