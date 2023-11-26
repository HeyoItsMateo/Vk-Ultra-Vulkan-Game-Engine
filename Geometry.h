#ifndef hGeometry
#define hGeometry

#include "Mesh.h"
#include "Utilities.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <thread>
#include <execution>

#include <numbers>
constexpr float phi = std::numbers::phi;

struct triPrim {
    uint16_t a, b, c;
};

namespace vk {
    namespace Geometry
    {
        struct Plane : test_Mesh {
            Plane(glm::vec2 Dimensions = { 1.f,1.f }, glm::vec2 Resolution = { 1.f,1.f });
        private:
            std::vector<triangleList> createVertices(glm::vec2 dimensions, glm::vec2 resolution);
            std::vector<uint16_t> createIndices(glm::vec2 dimensions);
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

        struct Icosahedron : test_Mesh {
            Icosahedron(float radius, int count) : test_Mesh(createVertices(radius), indices) {
                vertices = createVertices(radius);
                if (count) {
                    subdivide(radius, count);
                    test_Mesh::subdivide(vertices, indices);
                }
            }
        public:
            // Define the 12 vertices of an icosahedron
            std::vector<triangleList> vertices;
            inline static std::vector<triPrim> idx = {
                {5, 0, 4}, {4, 2, 5}, {5, 10, 0},
                {6, 1, 7}, {7, 3, 6},
                {8, 4, 0}, {0, 1, 8}, {8, 1, 6},
                {9, 8, 6}, {6, 3, 9}, {9, 3, 2}, {2, 4, 9}, {9, 4, 8},
                {10, 1, 0}, {10, 7, 1},
                {11, 2, 3}, {11, 3, 7}, {11, 7, 10}, {11, 10, 5}, {11, 5, 2}
            };
            inline static std::vector<uint16_t> indices = {
                5, 0, 4, 4, 2, 5, 5, 10, 0,
                6, 1, 7, 7, 3, 6,
                8, 4, 0, 0, 1, 8, 8, 1, 6,
                9, 8, 6, 6, 3, 9, 9, 3, 2, 2, 4, 9, 9, 4, 8,
                10, 1, 0, 10, 7, 1,
                11, 2, 3, 11, 3, 7, 11, 7, 10, 11, 10, 5, 11, 5, 2
            };
        private:
            void subdivide(float radius, int count) {
                size_t m = vertices.size();
                size_t n = indices.size();

                for (int i = 0; i < count; i++) {
                    size_t temp = indices.size();
                    for (uint16_t i = 0; i < temp; i += 3) {
                        uint16_t j = i + 1;
                        uint16_t k = i + 2;

                        m += 3;
                        n += 9;
                        vertices.resize(m);
                        indices.resize(n);

                        // Update the indices
                        uint16_t ai = m - 3;
                        uint16_t bi = m - 2;
                        uint16_t ci = m - 1;

                        std::vector<triangleList> newVtx =
                        {
                            midpoint(vertices[indices[i]], vertices[indices[j]], radius),
                            midpoint(vertices[indices[j]], vertices[indices[k]], radius),
                            midpoint(vertices[indices[k]], vertices[indices[i]], radius)
                        };
                        memcpy(&vertices[m - 3], newVtx.data(), newVtx.size() * sizeof(triangleList));

                        std::vector<uint16_t> newIdx =
                        {
                            indices[i], ai, ci,
                            ai, indices[j], bi,
                            ci, bi, indices[k]
                            //ai, bi, ci
                        };
                        indices[i] = ai;
                        indices[j] = bi;
                        indices[k] = ci;
                        memcpy(&indices[n - 9], newIdx.data(), newIdx.size() * sizeof(uint16_t));
                    }
                }
                std::cout << "vertices size = " << vertices.size() << std::endl;
                std::cout << "indices size = " << indices.size() << std::endl;
            }
            triangleList midpoint(triangleList& vertex_a, triangleList& vertex_b, float radius) {
                glm::vec3 a = normalize(glm::vec3(vertex_a.position));
                glm::vec3 b = normalize(glm::vec3(vertex_b.position));
                glm::vec3 ab = normalize(0.5f * (a + b));
                
                triangleList midPoint{
                    {radius * ab, 1.f},
                    normalize(midPoint.position),
                    lerp(vertex_a.color, vertex_b.color, 0.5f),
                    lerp(vertex_a.texCoord, vertex_b.texCoord, 0.5f)
                };
                return midPoint;
            }

            std::vector<triangleList> createVertices(float radius) {
                std::vector<triangleList> vtx(12); // Initialize size to 12 vertices of an Icosahedron
                for (int i = 0; i < 12; i++) {
                    vtx[i].position = { radius * normalize(glm::vec3(verts[i])), 1 };
                    vtx[i].normal = normalize(vtx[i].position);
                    vtx[i].color = vtx[i].normal;
                    vtx[i].texCoord = uvCoords[i];
                }
                return vtx;
            }
            std::vector<uint16_t> createIndices() {
                // Initialize the 20 triangular faces of an icosahedron
                std::vector<uint16_t> idx;
                for (const auto& face : faces) {
                    idx.push_back(face.x);
                    idx.push_back(face.y);
                    idx.push_back(face.z);
                }
                return idx;
            }
            inline static std::vector<triangleList> icoVerts{
                {// 0
                    {0, 1, phi, 1},
                    {normalize(glm::vec3(0,1,phi)), 1},
                    glm::vec4(1.f),
                    {0, 1} // * Radius
                },
                {// 2
                    {0, 1, -phi, 1},
                    {},
                    glm::vec4(1.f),
                    {0, (1 / 3)}
                },
                {// 11
                    {-phi, 0, -1, 1},
                    {},
                    glm::vec4(1.f),
                    {(1 / 5), (1 / 3)}
                },
                {// 7
                    {-1, -phi, 0, 1},
                    {},
                    glm::vec4(1.f),
                    {(2 / 5), (1 / 3)}
                },
                {// 6
                    {1, -phi, 0, 1},
                    {},
                    glm::vec4(1.f),
                    {(3 / 5), (1 / 3)}
                },
                {// 9
                    {phi, 0, -1, 1},
                    {},
                    glm::vec4(1.f),
                    {(4 / 5), (1 / 3)}
                },
                {// 5
                    {-1, phi, 0, 1},
                    {},
                    glm::vec4(1.f),
                    { (1 / 10), (2 / 3)}
                },
                {// 10
                    {-phi, 0, 1, 1},
                    {},
                    glm::vec4(1.f),
                    { (3/10), (2 / 3)}
                },
                {// 1
                    {0, -1, phi, 1},
                    {},
                    glm::vec4(1.f),
                    { (1/2), (2 / 3)}
                },
                {// 8
                    {phi, 0, 1, 1},
                    {},
                    glm::vec4(1.f),
                    { (7/10), (2 / 3)}
                },               
                {// 4
                    {1, phi, 0, 1},
                    {},
                    glm::vec4(1.f),
                    { (9/10), (2 / 3)}
                },
                {// 3
                    {0, -1, -phi, 1},
                    {normalize(glm::vec3(0,-1,-phi)), 1},
                    glm::vec4(1.f),
                    {(9 / 10), 0}
                }
            };

            inline static std::vector<glm::vec3> faces = {
                {8, 4, 0}, {5,4,2}, {4,5,0}, {8,0,1},
                {9, 3, 2}, {9, 6, 3}, {9, 8, 6}, {9, 4, 8},{9, 2, 4},
                {11, 2, 3}, {11, 3, 7}, {11, 5, 2}, {11, 10, 5},{10,11,7},
                {7, 6, 1}, {3,6,7}, {7,1,10}, {8,1,6}, {0,10,1}, {10,0,5}
            };

            inline static std::vector<glm::vec4> verts
            {
                {0, 1, phi, 1}, {0, -1, phi, 1}, {0, 1, -phi, 1}, {0, -1, -phi, 1},
                {1, phi, 0, 1}, {-1, phi, 0, 1}, {1, -phi, 0, 1}, {-1, -phi, 0, 1},
                {phi, 0, 1, 1}, {phi, 0, -1, 1}, {-phi, 0, 1, 1}, {-phi, 0, -1, 1},
            };
            inline static std::vector<glm::vec2> uvCoords{
                {0, 1}, {(1 / 2), (2 / 3)}, {0, (1 / 3)}, {(9 / 10), 0},
                {(9 / 10), (2 / 3)}, {(1 / 10), (2 / 3)}, {(3 / 5), (1 / 3)}, {(2 / 5), (1 / 3)},
                {(7 / 10), (2 / 3)}, {(4 / 5), (1 / 3)}, {(3 / 10), (2 / 3)}, {(1 / 5), (1 / 3)}
            };
        };

        struct test_graph {
            template <VkPrimitiveTopology topology>
            inline test_graph(std::vector<Primitive<topology>>& vertices) {
                size_t nVertices = vertices.size();
                adjacency.resize(nVertices * nVertices);
                for (size_t i = 0; i < nVertices; i++) {
                    for (size_t j = 0; j < nVertices; j++) {
                        size_t idx = j + (i * nVertices);
                        adjacency[idx] = glm::distance(vertices[i].position, vertices[j].position);
                    }
                }
            }
            std::vector<uint16_t> adjacency;
            std::vector<uint8_t> degree;
        };
    }
}

#endif