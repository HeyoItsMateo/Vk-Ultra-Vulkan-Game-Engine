#pragma once
#ifndef hPlanet
#define hPlanet

#include "Geometry.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
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
        Plate(float radius);
        Plate() = default;
    public:
        glm::vec3 position{ 0.f };  // Plate center
        //TODO: glm::vec3 movDir;   // Direction of movement
        glm::vec3 axis{ 0.f };      // Axis of rotation
        float omega = 0.f;          // Angular velocity

        glm::vec4 color{ 0.f };     // Debugging color
        std::vector<uint16_t> vtx_ids;
        std::vector<uint16_t> vtx_edge;

        //TODO: Shift Plate function
        // Shifts each plate's vertex position along drift direction.
        void shiftPlate(std::vector<triangleList>& vertices)
        {
            //TODO: Calculate gravitational erosion and adjust vertex heights
            // 
            //TODO: Convert shiftPlate() to a compute shader operation
            //  Update triangleList to a new type: plate_vertex
            //      Contains plate_id to access SSBO of plates for movement direction and density
            //  Update all vertices,
            // //TODO: After updating vertices, update the direction of motion of the plate itself.
            //  Calculate the direction of movement: 
            //      Sum up the direction of subducting vertices from the plate center (wiki: slab pull/slab suction)
            //      and add that to the plate's current movement direction.
            //      Sum up the direction of non-subducting vertices from the plate center (wiki: ridge push)
            //      and subtract that from the plate's current movement direction.
            //  then pass back to CPU to update VBO buffer with new/removed plates/vertices and plate info.

            float rotation = omega * vk::dt;
            glm::quat rotQuat = glm::angleAxis(rotation, axis);
            glm::quat cnjQuat = glm::conjugate(rotQuat);
            // Update plate position and movement direction.
            position = (rotQuat * position) * cnjQuat;
            //movDir = (rotQuat * movDir) * cnjQuat;
            // Update the plate's vertices.
            for (uint16_t i : vtx_ids)
            {
                vertices[i].position = shiftVertex(vertices[i].position, rotQuat, cnjQuat);
                //TODO: Track distance between plates of plate-edge vertices.
                    //  If the distance between plates is too large:
                    //      Check adjacent plate age to determine whether or not to form a new plate,
                    //      or to just add vertices to it.
                    //      Populate the new or already present plate with new vertices.
                    //      Adjust the new plate's movement by its size; smaller = slower
                    //  If the distance between plates is too small:
                    //      Test plate densities to decide which plate subducts.
                    //      Delete edge vertex of the subducting plate,
                    //      and assign a new plate-vertex as an edge vertex.
                    //      Adjust the height of plate-edge vertices to reflect the subduction
                    //      using the distance of overlap to determine the height change.
            }
            
            //TODO: Decide if wanting to add subsurface liquid-simulation to simulate mantle convection currents
            //      Add any effects on movement from mantle convection.
            //      Recalculate omega(angular velocity) and axis(axis of rotation = cross(platePos, movDir)) for the plate.

        }
        //TODO: Vertex update helper function
        static glm::vec4 shiftVertex(const glm::vec4& position, glm::quat& rotQuat, glm::quat& cnjQuat) {
            return (rotQuat * position) * cnjQuat;
        }

        void setWeights() {

        }

        
    protected:
        uint16_t plateID = 0;
    private:
        std::vector<float> Adjacency;
        //TODO: Change adjacency vector of floats to an adjacency list
        std::vector<float> Laplacian;
        
        static glm::vec3 generate_random_vector3(std::uniform_real_distribution<float>& xyz_distribution, std::uniform_real_distribution<float>& offset);

        static glm::vec4 generate_random_color4(float min = 0.1f, float max = 1.0f) {
            // Create distribution of RGB values.
            std::uniform_real_distribution<float> RGB(min, max);
            // Return random color.
            return { 
                RGB(global_rng), 
                RGB(global_rng), 
                RGB(global_rng), 
                1.f 
            };
        }

        friend struct Planet;
    };

    struct Planet : vk::Geometry::Icosahedron {
        Planet(uint16_t plate_count, float radius, int subdivisions)
            : Icosahedron(radius, subdivisions)
        {
            // Initialize the number of tectonic plates for the planet
            this->plate_count = plate_count;
            plates.resize(plate_count);
            // Create and initialize each plate tectonic
            for (int i = 0; i < plate_count; i++) {
                plates[i] = Plate(radius);
            }
            // Assign vertices to the nearest plate tectonic
            assignVertices(plate_count, vertices, plates, plate_ids);
            // Create adjacency matrix for adjacent tectonic plates
            connectPlates(indices);
            stageVBO->update(vertices.data(), VBO->buffer);
        }
    public:
        void updatePlates()
        {//TODO: dispatch to GPU with compute shader
            // Adjust each tectonic plate's position and modify the plate vertices.
            for (auto& plate : plates) {
                plate.shiftPlate(vertices);
            }
            //TODO: determine if connectPlates() is actually needed.
            //TODO: update connectPlates() to handle shifting plates
            //connectPlates(indices);

            // Update VBO with new vertex data.
            stageVBO->update(vertices.data(), VBO->buffer);
        }
    protected:
        Eigen::MatrixXi A;
        //TODO: Rewrite 'connectPlates()' to utilize an adjacency list
        //TODO: Determine if possible to dispatch to the GPU
        void connectPlates(std::vector<uint16_t>& indices)
        {// Maps triangle indices to plateIDs stored in the vertex map.
            A = Eigen::MatrixXi::Constant(plate_count, plate_count, 0);
            for (uint16_t i = 0; i < indices.size(); i+=3) {
                 uint16_t j = i + 1;
                 uint16_t k = i + 2;
                 // Get vertex_id from each triangle in the EBO's indices
                 uint16_t vtx_i = indices[i];
                 uint16_t vtx_j = indices[j];
                 uint16_t vtx_k = indices[k];
                 // Find which plate each vertex is associated with
                 uint16_t plate_i = plate_ids[vtx_i];
                 uint16_t plate_j = plate_ids[vtx_j];
                 uint16_t plate_k = plate_ids[vtx_k];
                 // Find and assign adjacent plates (without self-adjacency)
                 if ((plate_i != plate_j) and (plate_i != plate_k) and (plate_j != plate_k))
                 {//TODO: assign edge vertices to each plate, so it knows which vertices to perform calculations on
                     A(plate_i, plate_j) = 1;
                     A(plate_i, plate_k) = 1;
                     A(plate_j, plate_i) = 1;
                     A(plate_j, plate_k) = 1;
                     A(plate_k, plate_i) = 1;
                     A(plate_k, plate_j) = 1;
                 }
            }
            // Print adjacency matrix for debugging
            std::cout << A << std::endl;
        }
        void calcWeights()
        {//TODO: determine if A-matrix is even needed, also if this can be dispatched to the GPU.
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

        //TODO: Rewrite so vertices store both their plate ID, and the next nearest plate.
        static void assignVertices(uint16_t plate_count, std::vector<triangleList>& vertices, std::vector<Plate>& plates, std::vector<uint16_t>& vertex_map)
        {
            vertex_map.resize(vertices.size());
            for (uint16_t v = 0; v < vertices.size(); v++)
            {
                float min = glm::distance(glm::vec3(vertices[v].position), plates[0].position);
                vertices[v].color = plates[0].color;

                uint16_t plateIndex = 0;
                for (uint16_t i = 1; i < plate_count; i++)
                {
                    float distance = glm::distance(glm::vec3(vertices[v].position), plates[i].position);
                    if (distance < min)
                    {
                        // Set vertex color to it's corresponding plate color
                        vertices[v].color = plates[i].color;
                       
                        plateIndex = i;
                        min = distance;
                    }
                }
                // Assign vertices to their nearest plate.
                plates[plateIndex].vtx_ids.push_back(v); // Each plate knows which vertices are part of it
                //TODO: decide on alternative method
                //  plates[plateIndex].vertices.push_back(&vertices[v]);
                
                // Assign plate_id to the map of vertices.
                vertex_map[v] = plateIndex; // The planet object knows which plate each vertex is a part of
            }
        }
    };
}

#endif