#include "vk.instance.h"

namespace vk {
    Window::Window(const char* windowName, uint32_t WIDTH, uint32_t HEIGHT)
    {// Initialize and open application window
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        handle = glfwCreateWindow(WIDTH, HEIGHT, windowName, nullptr, nullptr);
        glfwSetWindowUserPointer(handle, this);
        glfwSetFramebufferSizeCallback(handle, framebufferResizeCallback);
    }
}
