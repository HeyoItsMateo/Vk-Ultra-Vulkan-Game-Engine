#pragma once
#ifndef hPlanet
#define hPlanet

#include "Geometry.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

/* For Planet Generation */
#include <random>
#include <functional>

struct plate {
    glm::vec4 color;
    glm::vec3 position;
    glm::vec3 rotAxis;
public:
    void shiftPlate() {
        float rotation = angularVelocity * vk::dt;
        glm::quat rotQuat = glm::angleAxis(rotation, rotAxis);
        glm::quat cnjQuat = glm::conjugate(rotQuat);
        for (int i = 0; i < vertices.size(); i++) {
            vertices[i]->position = (rotQuat * vertices[i]->position) * cnjQuat;
        }
    }
    std::vector<triangleList*> vertices;
private:
    friend struct Tectonics;
    friend struct Planet;
    float angularVelocity = 0.f;
};

struct Tectonics {
    Tectonics(float radius, int plate_count) {
        this->plate_count = plate_count;
        control_points.resize(plate_count);

        std::mt19937_64 rndEngine(rand());
        std::uniform_real_distribution<float> rndColor(0.1f, 1.0f);
        std::uniform_real_distribution<float> rndPos(-1, 1);
        std::uniform_real_distribution<float> offset(-0.25f, 0.25f);
        std::uniform_real_distribution<float> omega(-0.02f, 0.02f);

        for (auto& ctrl_point : control_points) {
            glm::vec3 position = { rndPos(rndEngine) + offset(rndEngine), rndPos(rndEngine) + offset(rndEngine),rndPos(rndEngine) + offset(rndEngine) };
            ctrl_point.position = radius * glm::normalize(position);
            ctrl_point.color = { rndColor(rndEngine), rndColor(rndEngine), rndColor(rndEngine), 1.f };

            glm::vec3 randOrtho = glm::normalize(glm::vec3(rndPos(rndEngine), rndPos(rndEngine), rndPos(rndEngine)));
            ctrl_point.rotAxis = glm::normalize(glm::cross(randOrtho, ctrl_point.position));
            ctrl_point.angularVelocity = omega(rndEngine);
        }
    }
public:
    std::vector<plate> control_points;
    void assignPlate(triangleList& vertex) {
        int plateIndex = 0;
        float min = glm::distance(glm::vec3(vertex.position), control_points[0].position);
        vertex.color = control_points[0].color;
        for (int i = 1; i < plate_count; i++) {
            float distance = glm::distance(glm::vec3(vertex.position), control_points[i].position);
            if (distance < min) {
                min = distance;
                vertex.color = glm::vec4(control_points[i].rotAxis, 1.f);
                //vertex.color = control_points[i].color;
                plateIndex = i;
            }
        }
        control_points[plateIndex].vertices.push_back(&vertex);
    }
private:
    int plate_count = 0;
    std::random_device rand;
};

namespace vk {
	struct Planet : Geometry::Icosahedron {
        Planet(float radius, int count)
            : Icosahedron(radius, count), plates(radius, 10) 
        {
            for (auto& vtx : vertices) {
                plates.assignPlate(vtx);
            }
            stageVBO->update(vertices.data(), VBO->buffer);
        }
    public:
        Tectonics plates;

        void updatePlates() {
            for (auto& ctrl_point : plates.control_points) {
                ctrl_point.shiftPlate();
            }
            stageVBO->update(vertices.data(), VBO->buffer);
        }
	};
}

#endif