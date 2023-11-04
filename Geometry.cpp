#include "Geometry.h"

namespace vk {
	namespace Geometry
	{
        /* Voxel Mesh */
        Voxel::Voxel(glm::vec4 Center, glm::vec4 Dimensions)
            : Mesh(createVertices(), createIndices())
        {
            center = Center;
            dimensions = Dimensions;
            matrix = glm::scale(glm::translate(matrix, glm::vec3(Center)), glm::vec3(Dimensions));
        }
        Voxel Voxel::operator()(glm::vec4 Center, glm::vec4 Dimensions) {
            return Voxel(Center, Dimensions);
        }
        std::vector<lineList> Voxel::createVertices()
        {
            std::vector<lineList> vertices(8);
            std::vector<glm::vec4> metrics = { { 1,  1,  1,  1 },
                                               { 1,  1, -1,  1 },
                                               { 1, -1, -1,  1 },
                                               { 1, -1,  1,  1 } };
            for (int i = 0; i < 8; i++) {
                if (i < 4) {
                    vertices[i].position = metrics[i];
                    vertices[i].normal = metrics[i];
                    vertices[i].color = glm::vec4(1.f);
                    vertices[i].texCoord = glm::vec2(0.f);
                }
                else {
                    metrics[i - 4] *= glm::vec4(-1, 1, 1, 1);
                    vertices[i].position = metrics[i - 4];
                    vertices[i].normal = metrics[i - 4];
                    vertices[i].color = glm::vec4(1.f);
                    vertices[i].texCoord = glm::vec2(0.f);
                }
            }
            return vertices;
        }
        std::vector<uint16_t> Voxel::createIndices()
        {
            std::vector<uint16_t> indices(24);
            indices[7] = 0;
            indices[15] = 4;
            for (int x = 0; x < 7; x++) {
                indices[x] = static_cast<uint16_t>(floor((x + 1) / 2));
                indices[x + 8] = static_cast<uint16_t>(floor((x + 1) / 2) + 4);
            }
            int temp0 = 4, temp1 = 0;
            for (int x = 16; x < 24; x += 2) {
                indices[x] = temp0;
                indices[x + 1] = temp1;

                temp0++;
                temp1++;
            }
            return indices;
        }

        /* Plane Mesh */
        Plane::Plane(glm::vec2 Dimensions, glm::vec2 Resolution)
            : test_Mesh(createVertices(Dimensions, Resolution), createIndices(Dimensions))
        {
            glm::vec2 temp = Dimensions * Resolution;
            matrix = glm::translate(matrix, glm::vec3(-temp.x / 2.f, 0.f, -temp.y / 2.f));
        }
        std::vector<triangleList> Plane::createVertices(glm::vec2 dimensions, glm::vec2 resolution)
        {
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
        std::vector<uint16_t> Plane::createIndices(glm::vec2 dimensions)
        {
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
}
