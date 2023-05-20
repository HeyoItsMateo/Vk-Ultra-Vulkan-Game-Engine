#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct appWindow {
	GLFWwindow* window{}; //GLFW window handle

	appWindow(const char *windowName, uint32_t WIDTH = 800, uint32_t HEIGHT = 600)
	{// Initialize and open application window
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, windowName, nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}
	virtual ~appWindow()
	{// Destroy the structure and free memory resources
		glfwDestroyWindow(window);
		glfwTerminate();
	}

private:
	bool framebufferResized = false;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<appWindow*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

};