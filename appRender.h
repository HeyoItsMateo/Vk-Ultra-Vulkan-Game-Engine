#include "appWindow.h"

class appRender : appWindow {
public:
	appRender(const char* windowName, uint32_t WIDTH = 800, uint32_t HEIGHT = 600) : appWindow(windowName, WIDTH, HEIGHT)
	{// Initialize render application
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}
	virtual ~appRender()
	{// Destroy the render application

	}
};