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
inline std::map<int, void(*)()> keyboard_map
{
    { GLFW_KEY_ESCAPE, close }
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
        void Console_Input();
        void Mouse_Input();
        void Keyboard_Input();
    public:
        void update(float FOVdeg, float nearPlane, float farPlane);
    protected:
        bool noClip = true;
        bool firstClick = true;

        float velocity = 0.05f;
        float sensitivity = 1.75f;
        glm::vec3 Orientation;
        glm::vec3 Up;
    };
}

#endif