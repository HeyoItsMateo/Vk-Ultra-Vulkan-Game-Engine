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
vk::UBO ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

#include "Plane.h"
#include "Icosahedron.h"
#include "Particles.h"

vk::Scene world[] = {
    //{ planePPL, plane },
    { icoPPL, icosphere },
};

vk::Shader planeCompute("plane.comp", VK_SHADER_STAGE_COMPUTE_BIT);
vk::Shader particleCompute("point.comp", VK_SHADER_STAGE_COMPUTE_BIT);
vk::ComputePPL computePPL[] = {
    //{ planeCompute, planeSet, planeLayout, {heightMap.extent.width/100, heightMap.extent.height/100, 1 } },
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

std::vector<uint16_t> test_idx = {
                5, 0, 4, 4, 2, 5, 5, 10, 0,
                6, 1, 7, 7, 3, 6,
                8, 4, 0, 0, 1, 8, 8, 1, 6,
                9, 8, 6, 6, 3, 9, 9, 3, 2, 2, 4, 9, 9, 4, 8,
                10, 1, 0, 10, 7, 1,
                11, 2, 3, 11, 3, 7, 11, 7, 10, 11, 10, 5, 11, 5, 2
};
std::vector<triangleList> test_vtx(12);

struct test_memcpy {
    test_memcpy(std::vector<triangleList>& vertices, std::vector<uint16_t>& indices, uint16_t count = 1) {
        size_t m = vertices.size();
        size_t n = indices.size();
        size_t temp = indices.size();
        for (int i = 0; i < count; i++) {
            std::vector<triPrim> newIndices;
            for (uint16_t i = 0; i < temp; i += 3) {
                uint16_t j = i + 1;
                uint16_t k = i + 2;

                m += 3;
                n += 12;
                vertices.resize(m);
                indices.resize(n);
                
                // Update the indices
                uint16_t ai = m - 3;
                uint16_t bi = m - 2;
                uint16_t ci = m - 1;
                std::vector<uint16_t> newIdx = 
                {
                    indices[i], ai, ci,
                    ai, indices[j], bi,
                    ci, bi, indices[k],
                    ai, bi, ci
                };

                memcpy(&indices[n - 12], newIdx.data(), newIdx.size() * sizeof(uint16_t));
            }
        }
    }
};

int main() {
    //vk::Geometry::test_graph testGraph(icosphere.vertices);
    test_memcpy testing(test_vtx, test_idx);
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
            icosphere.updatePlates();
        }
        vkDeviceWaitIdle(vk::GPU::device);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
