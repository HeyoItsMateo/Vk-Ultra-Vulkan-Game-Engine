#include "vk.engine.h"
#include "vk.textures.h"

#include <algorithm>
#include <functional>

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

//TODO: Learn discrete differential geometry
//TODO: Implement waves~
//TODO: Optimize the swapchain and rendering process
//TODO: Optimize a fuckload of stuff with shader caching, pipeline caching, parallelization, etc.

vk::Window window("Vulkan");
vk::Engine app;

vk::Uniforms uniforms;
vk::UBO ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

#include "Plane.h"
#include "Icosahedron.h"
#include "Particles.h"

vk::Scene world[] = {
    { planePPL, plane },
    { icoPPL, icosphere }
};

vk::Shader planeCompute("plane.comp", VK_SHADER_STAGE_COMPUTE_BIT);
vk::Shader particleCompute("point.comp", VK_SHADER_STAGE_COMPUTE_BIT);
vk::ComputePPL computePPL[] = {
    { planeCompute, planeSet, planeLayout, {heightMap.extent.width/100, heightMap.extent.height/100, 1 } },
    { particleCompute, pointSet, pointLayout, {100, 100, 10} }
};

//double mouseX;
//double mouseY;
//void trackMouse(double x, double y) {
//    // Fetches and stores the coordinates of the cursor
//    glfwGetCursorPos(vk::Window::handle, &x, &y);
//    std::cout << "Mouse position: (" << x << ", " << y << ")" << std::endl;
//}
//
//#include <map>
//#include <glm/gtx/hash.hpp>
//std::unordered_map<glm::vec4, bool> testMap = {
//    { { 1, 0, 0, 0 }, true },
//    { { 0, 0, 1, 0 }, true }
//};
//
//// Hash bin testing
//#include <random>
//std::vector<glm::vec3> positions{ 5 };
//glm::vec3 avgPos{ 0.f };

int main() {
    try {
        glfwSetKeyCallback(vk::Window::handle, userInput);
        //auto* instance = static_cast<vk::Camera*>(glfwGetWindowUserPointer(vk::Window::handle));
        //if (instance) {
        //    /* do stuff */
        //}

        while (!glfwWindowShouldClose(vk::Window::handle)) {
            glfwPollEvents();
            //std::jthread tMouse(trackMouse, mouseX, mouseY);

            ubo.update(uniforms);
            app.run(world, computePPL, particlePPL, ssbo);
        }
        vkDeviceWaitIdle(vk::GPU::device);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
