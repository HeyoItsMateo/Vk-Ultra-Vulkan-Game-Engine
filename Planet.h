#pragma once
#ifndef hPlanet
#define hPlanet

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "Geometry.h"

/* For Linear Algebra */
#include <Eigen/Dense>

struct Plate {
    Plate(float radius);
    Plate() = default;
public:
    glm::vec3 position{ 0.f };
    glm::vec3 axis{ 0.f };
    glm::vec4 color{ 0.f };
    float omega = 0.f;

    std::set<uint16_t> vtx_ids; //Contains all vertex IDs associated with the plate
    std::set<uint16_t> vtx_edge; //Contains the vertex index for edge verticess
protected:
    Eigen::MatrixXi vtx_incidence; //Contains the vertex 

    Eigen::MatrixXi vtx_adjacency;
    Eigen::MatrixXi vtx_degree;
    Eigen::MatrixXi vtx_laplacian;

    uint16_t plateID = 0;
private:
    friend struct Planet;

    void init_pos(float radius, float min, float max, float offset);
    void init_rgb(float min, float max, float offset);
    void init_rot(float min, float max);

    

    void createIncidenceMatrix(Eigen::MatrixXi& plate_adjacency) {

    }

    glm::quat rotQuat() const {
        float rotation = (omega * vk::dt);
        return glm::angleAxis(rotation, axis);
    }
    glm::vec4 shiftVertex(const glm::vec4& position) const {
        glm::quat rotation = rotQuat();
        glm::quat cnjQuat = glm::conjugate(rotation);
        return (rotation * position) * cnjQuat;
    }

};


struct Planet : vk::Geometry::Icosahedron {
    Planet(uint16_t plate_count, float radius, int subdivisions);
    ~Planet();

public:
    void updatePlates();

protected:
    float radius;
    uint16_t plate_count = 0;
    std::vector<Plate> plates;

    Eigen::MatrixXi plate_adjacency;
    Eigen::MatrixXi plate_degree;
    Eigen::MatrixXi plate_laplacian;
private:
    uint16_t* vtx_plate_map; // vtx_plate_map[vertex index] returns the index of the plate which the vertex is assigned to

    std::vector<uint16_t> plate_vtx_map; // plate-vertex map
    std::vector<uint16_t> plate_idx_map;

    void assignVerts();
    void connectPlates(std::vector<uint16_t>& indices);
    void createPlateIncidence(std::vector<uint16_t>& indices);

    void calcWeights() {
        for (uint16_t i = 0; i < plate_count; i++) {
            for (uint16_t j = 0; j < plate_count; j++) {
                if (plate_adjacency(i, j)) {
                    glm::quat dq = plates[i].rotQuat() * plates[j].rotQuat();

                }
            }
        }

    }
};

#endif