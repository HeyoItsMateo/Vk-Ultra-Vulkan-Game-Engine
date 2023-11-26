#pragma once
#ifndef hPlanet
#define hPlanet

#include "Geometry.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

/* For Planet Generation */
#include <thread>
#include <random>
#include <functional>

/* For Linear Algebra */
#include <Eigen/Dense>



struct Adjacency {
    Adjacency(std::vector<triangleList>& vertices) {
        uint16_t size = vertices.size();
        uint16_t linear_size = size * size;
        matrix.resize(linear_size);

        for (const auto& vtx : vertices) {
            for (const auto& vty : vertices) {
                if (&vtx != &vty) {

                }
            }
        }
    }
    std::vector<int> matrix;
};

namespace vk {
    

    
}

/*Proceedural Generation Library*/
namespace pgl {
    static std::random_device random;
    inline thread_local std::mt19937 global_rng{ random() };

    struct Plate {
        glm::vec3 position { 0.f };
        glm::vec4 color { 0.f };
        
        Plate(float radius) {
            std::uniform_real_distribution<float> RGB( 0.1f, 1.0f);
            std::uniform_real_distribution<float> XYZ(-1.0f, 1.0f);
            std::uniform_real_distribution<float> offset(-0.25f, 0.25f);
            std::uniform_real_distribution<float> angularVelocity(-0.02f, 0.02f);

            std::thread t_pos([this, radius, XYZ, offset] { init_pos(radius, XYZ, offset); });
            std::jthread t_rgb([this, RGB] { init_rgb(RGB); });

            glm::vec3 randOrtho{
                XYZ(global_rng),
                XYZ(global_rng),
                XYZ(global_rng)
            };
            t_pos.join();
            axis = glm::normalize(glm::cross(randOrtho, position));
            omega  = angularVelocity(global_rng);
            //plateID = i;
        }
        Plate() = default;
    public:
        float omega = 0.f;
        glm::vec3 axis{ 0.f };

        Eigen::MatrixXf vertexGraph;

        glm::vec4 shiftVertex(const glm::vec4& position) {
            float rotation = omega * vk::dt;
            glm::quat rotQuat = glm::angleAxis(rotation, axis);

            glm::quat cnjQuat = glm::conjugate(rotQuat);
            return(rotQuat * position) * cnjQuat;
        }

        void setWeights() {

        }

        std::vector<uint16_t> vtx_ids;
        std::vector<uint16_t> vtx_edge;
    protected:
        uint16_t plateID = 0;
    private:
        friend struct Planet;

        void init_pos(float radius, std::uniform_real_distribution<float> distribution, std::uniform_real_distribution<float> offset) {
            glm::vec3 pos {
                distribution(global_rng) + offset(global_rng),
                distribution(global_rng) + offset(global_rng),
                distribution(global_rng) + offset(global_rng)
            };
            position = radius * glm::normalize(pos);
        }
        void init_rgb(std::uniform_real_distribution<float> RGB) {
            color = { RGB(global_rng), RGB(global_rng), RGB(global_rng), 1.f };
        }

        std::vector<float> Adjacency;
        std::vector<float> Laplacian;
    };

    struct Planet : vk::Geometry::Icosahedron {
        Planet(uint16_t plate_count, float radius, int subdivisions)
            : Icosahedron(radius, subdivisions)
        {
            this->plate_count = plate_count;
            plates.resize(plate_count);
            for (int i = 0; i < plate_count; i++) {
                plates[i] = Plate(radius);
            }
            assignVertices(plate_count, vertices, plates, plate_ids);

            connectPlates(indices);
            stageVBO->update(vertices.data(), VBO->buffer);
        }
    public:
        void updatePlates() {
            for (auto& plate : plates) {//TODO: dispatch to GPU with compute shader
                
                for (uint16_t& i : plate.vtx_ids) {
                    //vertices[i].position = plate.shiftVertex(vertices[i].position);
                }
            }
            stageVBO->update(vertices.data(), VBO->buffer);
        }
    protected:
        Eigen::MatrixXi A;
        void connectPlates(std::vector<uint16_t>& indices)
        {// Maps triangle indices to plateIDs stored in the vertex map.
            A = Eigen::MatrixXi::Constant(plate_count, plate_count, 0);
            for (uint16_t i = 0; i < indices.size(); i+=3) {
                 uint16_t j = i + 1;
                 uint16_t k = i + 2;
                 uint16_t vtx_i = indices[i];
                 uint16_t vtx_j = indices[i + 1];
                 uint16_t vtx_k = indices[i + 2];

                 uint16_t plate_i = plate_ids[vtx_i];
                 uint16_t plate_j = plate_ids[vtx_j];
                 uint16_t plate_k = plate_ids[vtx_k];

                 if ((plate_i != plate_j) and (plate_i != plate_k) and (plate_j != plate_k)) {
                     A(plate_i, plate_j) = 1;
                     A(plate_i, plate_k) = 1;
                     A(plate_j, plate_i) = 1;
                     A(plate_j, plate_k) = 1;
                     A(plate_k, plate_i) = 1;
                     A(plate_k, plate_j) = 1;
                 }
            }
            std::cout << A << std::endl;
        }
        void calcWeights() {
            for (uint16_t i = 0; i < plate_count; i++) {
                for (uint16_t j = 0; j < plate_count; j++) {
                    if (A(i, j)) {

                    }
                }
            }

        }
        void runSim() {

        }
    private:
        int plate_count = 0;
        std::vector<Plate> plates;
        std::vector<uint16_t> plate_ids;
        std::vector<uint16_t> index_map;

        static void assignVertices(uint16_t plate_count, std::vector<triangleList>& vertices, std::vector<Plate>& plates, std::vector<uint16_t>& vertex_map)
        {
            vertex_map.resize(vertices.size());
            for (uint16_t v = 0; v < vertices.size(); v++)
            {
                float min = glm::distance(glm::vec3(vertices[v].position), plates[0].position);

                uint16_t plateIndex = 0;
                for (uint16_t i = 1; i < plate_count; i++)
                {
                    float distance = glm::distance(glm::vec3(vertices[v].position), plates[i].position);
                    if (distance < min)
                    {
                        vertices[v].color = glm::vec4(plates[i].axis, 1.f);
                       
                        plateIndex = i;
                        min = distance;
                    }
                }
                //plates[plateIndex].vertices.push_back(&vertices[v]);
                plates[plateIndex].vtx_ids.push_back(v);
                vertex_map[v] = plateIndex;
            }
        }
    };
}

#endif