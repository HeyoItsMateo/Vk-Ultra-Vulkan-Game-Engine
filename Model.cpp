#include "Model.h"

namespace vk {
    Plane::Plane(glm::vec2 Dimensions, glm::vec2 Resolution)
        : Mesh(createVertices(Dimensions, Resolution), createIndices(Dimensions))
    {
        glm::vec2 temp = Dimensions * Resolution;
        matrix = glm::translate(matrix, glm::vec3(-temp.x / 2.f, 0.f, -temp.y / 2.f));
    }
    /* Private */
    std::vector<triangleList> Plane::createVertices(glm::vec2 dimensions, glm::vec2 resolution) {
        std::vector<triangleList> vertices;
        triangleList vertex;
        for (int i = 0; i < dimensions.x; i++) {
            for (int j = 0; j < dimensions.y; j++) {
                vertex.position = { i * resolution.x, 0, j * resolution.y, 1 };
                vertex.normal = { 0, 0, 1, 1 };
                vertex.color = { 0,1,0,1 };
                vertex.texCoord = { i, j };
                vertices.push_back(vertex);
            }
        }
        return vertices;
    }
    std::vector<uint16_t> Plane::createIndices(glm::vec2 dimensions) {
        std::vector<uint16_t> indices;
        for (int i = 0; i < dimensions.x - 1; i++) {
            int column = (dimensions.y * i);
            int row = (dimensions.y * (i + 1));
            for (int j = 0; j < dimensions.y - 1; j++) {
                indices.push_back(row + j + 1);
                indices.push_back(row + j);
                indices.push_back(column + j);
                indices.push_back(column + j);
                indices.push_back(column + j + 1);
                indices.push_back(row + j + 1);
            }
        }
        return indices;
    }
}