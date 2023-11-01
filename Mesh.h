#ifndef hModel
#define hModel

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vk.buffers.h"
#include "vk.primitives.h"

#include <thread>
#include <execution>

#include <numbers>
constexpr float phi = std::numbers::phi;

namespace vk {
    struct Mesh {
        alignas (16) glm::mat4 matrix = glm::mat4(1.f);
        template <typename T>
        inline Mesh(std::vector<T> vertices, std::vector<uint16_t> indices) 
            : indexCount(static_cast<uint32_t>(indices.size())),
            VBO(vertices.size() * sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
            EBO(indices.size() * sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
            stageVBO(vertices.data(), VBO.size),
            stageEBO(indices.data(), EBO.size)
        {
            stageVBO.transferData(VBO.buffer);
            stageEBO.transferData(EBO.buffer);
        }
        ~Mesh() = default;
    public:
        Buffer VBO, EBO;
        template<typename T>
        inline void update(std::vector<T>& vertices, std::vector<uint16_t>& indices) {
            stageVBO.update(vertices, VBO.buffer);
            stageEBO.update(indices, EBO.buffer);
        }
        template<typename T>
        inline void subdivide(std::vector<T>& vertices, std::vector<uint16_t>& indices) {
            VBO.~Buffer();
            EBO.~Buffer();
            stageVBO.~StageBuffer();
            stageEBO.~StageBuffer();

            indexCount = static_cast<uint32_t>(indices.size());

            VBO = new Buffer(vertices.size() * sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            EBO = new Buffer(indices.size() * sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            stageVBO = new StageBuffer(vertices.data(), VBO.size);
            stageEBO = new StageBuffer(indices.data(), EBO.size);
        }
        void draw(uint32_t instanceCount = 1) {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VBO.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, EBO.buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
        }
    protected:
        StageBuffer stageVBO, stageEBO;
        uint32_t indexCount;
    };

    struct Collider
    {//TODO: create collision mesh
        // Options to include: default cube, sphere, or low-poly mesh
        //  Goal: reduce interacting vertices but maintain object form
    };

    struct Voxel : Mesh {
        glm::vec4 center;
        glm::vec4 dimensions{ 1.f };
        Voxel(glm::vec4 Center = { 0,0,0,1 }, glm::vec4 Dimensions = { 1,1,1,1 });
        Voxel operator()(glm::vec4 Center = { 0,0,0,1 }, glm::vec4 Dimensions = { 1,1,1,1 });
    public:
        // Octree Spacial Data Structure
        // TODO: rethink octree code
        //  works fine, but could be improved
        /*
        bool containsVertex = false;
        template <typename T>
        inline void checkVoxel(std::vector<T>& vertices) {
            for (auto& vertex : vertices) {
                float dist = glm::distance(vertex.position, center);
                float dotp = glm::dot(center, vertex.position);

                if ((0.5f <= dotp <= 1.5f) and (dist <= glm::length(dimensions))) {
                    content.insert(&vertex);
                    containsVertex = true;
                    break;
                }
            }
        }
        void checkNode() {
            for (auto& vert : content) {
                Primitive v = *vert;
                float dist = glm::distance(v.position, center);
                float dotp = glm::dot(center, v.position);

                if ((dotp < 0.5f) or (dotp > 1.5f) or (dist > glm::length(dimensions))) {
                    content.erase(vert);
                    std::cout << "Erased a vertex!\n";
                }
            }
        }
        */
    private:
        std::vector<lineList> createVertices();
        std::vector<uint16_t> createIndices();
    };

    struct Plane : Mesh {
        Plane(glm::vec2 Dimensions = {1.f,1.f}, glm::vec2 Resolution = {1.f,1.f});
    private:
        std::vector<triangleList> createVertices(glm::vec2 dimensions, glm::vec2 resolution);
        std::vector<uint16_t> createIndices(glm::vec2 dimensions);
    };

    struct hashGrid : Mesh {
        hashGrid(glm::vec3 dimensions, glm::vec3 resolution)
            : Mesh(createVertices(dimensions, resolution), createIndices(dimensions))
        {

        }
    private:
        std::vector<lineList> createVertices(glm::vec3 dimensions, glm::vec3 resolution);
        std::vector<uint16_t> createIndices(glm::vec3 dimensions);
    };

    /*
    std::vector<triangleList> Icosahedron::updateVertices(float radius, int count) {
        // Subdivide the faces
        std::vector<triangleList> vertices_;
        triangleList vertex[3];
        for (int i = 0; i < count; i++) {
            std::vector<glm::vec3> newFaces;
            for (const auto& face : faces) {
                glm::vec4 a = normalize(vertices[face.x].position);
                glm::vec4 b = normalize(vertices[face.y].position);
                glm::vec4 c = normalize(vertices[face.z].position);

                // Midpoints
                glm::vec4 ab = normalize(0.5f * (a + b));
                glm::vec4 bc = normalize(0.5f * (b + c));
                glm::vec4 ca = normalize(0.5f * (c + a));
                vertex[0] = { ab, ab, glm::vec4(1.f), glm::vec2(1.f) };
                vertex[1] = { bc, bc, glm::vec4(1.f), glm::vec2(1.f) };
                vertex[2] = { ca, ca, glm::vec4(1.f), glm::vec2(1.f) };
                // Add new vertices
                vertices_.push_back(vertex[0]);
                vertices_.push_back(vertex[1]);
                vertices_.push_back(vertex[2]);

                // Update the faces
                uint16_t ai = vertices_.size() - 3;
                uint16_t bi = vertices_.size() - 2;
                uint16_t ci = vertices_.size() - 1;

                newFaces.push_back({ face.x, ai, ci });
                newFaces.push_back({ ai, face.y, bi });
                newFaces.push_back({ bi, face.z, ci });
                newFaces.push_back({ ai, bi, ci });
            }
            faces = newFaces;
        }
}*/

    struct Icosahedron : Mesh {
        
        Icosahedron(float radius, int count) : Mesh(createVertices(count), createIndices()) {
            matrix = glm::scale(matrix, glm::vec3(radius));
        }
    private:
        std::vector<triangleList> createVertices(int count) {
            std::vector<triangleList> vertices;
            triangleList vtx;
            for (const auto& pos : verts) {
                vtx.position = pos;
                vtx.normal = normalize(pos);
                vtx.color = glm::abs(vtx.normal);
                vtx.texCoord = glm::vec2(1.f);

                vertices.push_back(vtx);
            }
            return vertices;
        }
        std::vector<uint16_t> createIndices() {
            // Initialize the 20 triangular faces of an icosahedron
            std::vector<uint16_t> indices;
            for (const auto& face : faces) {
                indices.push_back(face.x);
                indices.push_back(face.y);
                indices.push_back(face.z);
            }
            return indices;
        }
        // Define the 12 vertices of an icosahedron
        
        inline static std::vector<glm::vec3> faces = {
            {8, 4, 0}, {5,4,2}, {4,5,0},{8,0,1},
            {9, 3, 2}, {9, 6, 3}, {9, 8, 6}, {9, 4, 8},{9, 2, 4},
            {11, 2, 3}, {11, 3, 7}, {11, 5, 2}, {11, 10, 5},{10,11,7},
            {7, 6, 1},{3,6,7}, {7,1,10}, {8,1,6}, {0,10,1}, {10,0,5}
        };
        inline static std::vector<glm::vec4> verts 
        {
            {0, 1, phi, 1}, {0, -1, phi, 1}, {0, 1, -phi, 1}, {0, -1, -phi, 1},
            {1, phi, 0, 1}, {-1, phi, 0, 1}, {1, -phi, 0, 1}, {-1, -phi, 0, 1},
            {phi, 0, 1, 1}, {phi, 0, -1, 1}, {-phi, 0, 1, 1}, {-phi, 0, -1, 1},
        };
    };

}

#endif