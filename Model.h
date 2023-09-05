#ifndef hModel
#define hModel

#include "Buffers.h"

namespace vk {
    struct GameObject : VectorBuffer<Vertex>, VectorBuffer<uint16_t> {

        void draw() {
            VkCommandBuffer& commandBuffer = vk::EngineCPU::renderCommands[vk::SwapChain::currentFrame];

            VectorBuffer<Vertex>::bind(commandBuffer);
            VectorBuffer<uint16_t>::bind(commandBuffer);

            vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
        }
    protected:
        uint32_t indexCount, instanceCount = 1;
        void init_GameObject(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) {
            VectorBuffer<Vertex>::init(vertices);
            VectorBuffer<uint16_t>::init(indices);
            indexCount = static_cast<uint32_t>(indices.size());
        }
    };

    struct Plane : GameObject {
        Plane(glm::vec2 Dimensions, glm::vec2 Resolution) {
            createVertices(Dimensions, Resolution);
            createIndices(Dimensions);
            VectorBuffer<Vertex>::init(vertices);
            VectorBuffer<uint16_t>::init(indices);
            indexCount = static_cast<uint32_t>(indices.size());
        }
    public:
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    private:
        void createVertices(glm::vec2 dimensions, glm::vec2 resolution) {
            for (int i = 0; i < dimensions.x; i++) {
                for (int j = 0; j < dimensions.y; j++) {
                    vertices.push_back(Vertex{ { i * resolution.x, 0, j * resolution.y, 1 }, {0,1,0,1}, {i,j} });
                }
            }
        }
        void createIndices(glm::vec2 dimensions) {
            //indices.resize(6 * (dimensions.x * dimensions.y));
            for (int i = 0; i < dimensions.x-1; i++) {
                int column = (dimensions.y * i);
                int row = (dimensions.y * (i + 1));
                for (int j = 0; j < dimensions.y-1; j++) {
                    indices.push_back(row + j + 1);
                    indices.push_back(row + j);
                    indices.push_back(column + j);
                    indices.push_back(column + j);
                    indices.push_back(column + j + 1);
                    indices.push_back(row + j + 1);
                }
            }
        }
    };

    struct Model : GameObject {
        Model(std::vector<Vertex> const& vtx, std::vector<uint16_t> const& idx) {
            VectorBuffer<Vertex>::init(vtx);
            VectorBuffer<uint16_t>::init(idx);
            indexCount = static_cast<uint32_t>(idx.size());
        }
    public:
        void update(std::vector<Vertex> const& vtx) { //TODO: create Update Func
            VectorBuffer<Vertex>::update(vtx);
        }
    };
}

#endif