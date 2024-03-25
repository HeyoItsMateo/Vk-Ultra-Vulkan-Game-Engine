#include "Planet.h"
#include "ProcGenLib.h"

// Constructor
Planet::Planet(uint16_t plate_count, float radius, int subdivisions)
    : Icosahedron(radius, subdivisions)
{
    vtx_plate_map = new uint16_t[vertices.size()];

    this->plate_count = plate_count;
    plates.resize(plate_count);
    for (int i = 0; i < plate_count; i++) {
        plates[i] = Plate(radius);
    }

    assignVerts();
    connectPlates(indices);
    createPlateIncidence(indices);
    stageVBO->update(vertices.data(), VBO->buffer);
}
// Destructor
Planet::~Planet()
{
    delete vtx_plate_map;
}
// Function to move tectonic plates
void Planet::updatePlates()
{
    for (auto& plate : plates) {//TODO: dispatch to GPU with compute shader

        for (const uint16_t i : plate.vtx_ids) {
            vertices[i].position = plate.shiftVertex(vertices[i].position);
        }
    }
    stageVBO->update(vertices.data(), VBO->buffer);
}
// to assign vertices to their nearest plate
void Planet::assignVerts() {
    int vtx_id = 0;
    for (triangleList& vtx : vertices) {
        float min = glm::distance(glm::vec3(vtx.position), plates[0].position);

        uint16_t plateIndex = 0;
        for (uint16_t i = 1; i < plate_count; i++)
        {
            float distance = glm::distance(glm::vec3(vtx.position), plates[i].position);
            if (distance < min)
            {
                vtx.color = plates[i].color;
                //vtx.color = glm::vec4(plates[i].axis, 1.f);

                plateIndex = i;
                min = distance;
            }
        }
        plates[plateIndex].vtx_ids.insert(vtx_id);

        vtx_plate_map[vtx_id] = plateIndex;
        vtx_id++;
    }
}

void Planet::connectPlates(std::vector<uint16_t>& indices)
{// Maps triangle indices to plateIDs stored in the vertex map.
    plate_adjacency = Eigen::MatrixXi::Constant(plate_count, plate_count, 0);

    for (size_t i = 0; i < indices.size(); i += 3) {
        // Obtain index of vertex_ijk associated with each triangle face
        uint16_t vtx_i = indices[i];
        uint16_t vtx_j = indices[i + 1];
        uint16_t vtx_k = indices[i + 2];

        // Find plate_ijk associated with vertex_ijk, for each triangle face
        uint16_t plate_i = vtx_plate_map[vtx_i];
        uint16_t plate_j = vtx_plate_map[vtx_j];
        uint16_t plate_k = vtx_plate_map[vtx_k];
        // Find plate edge vertices
        if ((plate_i != plate_j) or (plate_i != plate_k) or (plate_j != plate_k)) {
            plates[plate_i].vtx_edge.insert(vtx_i);
            plates[plate_j].vtx_edge.insert(vtx_j);
            plates[plate_k].vtx_edge.insert(vtx_k);
            // Create plate adjacency matrix
            if ((plate_i != plate_j) and (plate_i != plate_k) and (plate_j != plate_k)) {
                //TODO: create individal plate adjacency matrices here
                // 
                plate_adjacency(plate_i, plate_j) = 1;
                plate_adjacency(plate_i, plate_k) = 1;
                plate_adjacency(plate_j, plate_i) = 1;
                plate_adjacency(plate_j, plate_k) = 1;
                plate_adjacency(plate_k, plate_i) = 1;
                plate_adjacency(plate_k, plate_j) = 1;
            }
        }
    }
    plate_degree = plate_adjacency.colwise().sum().asDiagonal();
    plate_laplacian = plate_degree + plate_adjacency;

    std::cout << "plate laplacian:\n";
    std::cout << plate_laplacian << std::endl;
}

void Planet::createPlateIncidence(std::vector<uint16_t>& indices) {
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Obtain index of vertex_ijk associated with each triangle face
        uint16_t vtx_i = indices[i];
        uint16_t vtx_j = indices[i + 1];
        uint16_t vtx_k = indices[i + 2];

        // Find plate_ijk associated with vertex_ijk, for each triangle face
        uint16_t plate_i = vtx_plate_map[vtx_i];
        uint16_t plate_j = vtx_plate_map[vtx_j];
        uint16_t plate_k = vtx_plate_map[vtx_k];
        // Find plate edge vertices
        if ((plate_i != plate_j) or (plate_i != plate_k) or (plate_j != plate_k)) {
            plates[plate_i].vtx_edge.insert(vtx_i);
            plates[plate_j].vtx_edge.insert(vtx_j);
            plates[plate_k].vtx_edge.insert(vtx_k);
            // Create plate adjacency matrix
            if ((plate_i != plate_j) and (plate_i != plate_k) and (plate_j != plate_k)) {
                //TODO: create individal plate adjacency matrices here
                // 
                plate_adjacency(plate_i, plate_j) = 1;
                plate_adjacency(plate_i, plate_k) = 1;
                plate_adjacency(plate_j, plate_i) = 1;
                plate_adjacency(plate_j, plate_k) = 1;
                plate_adjacency(plate_k, plate_i) = 1;
                plate_adjacency(plate_k, plate_j) = 1;
            }
        }
    }
    // Create edge weights using incidence matrix
    for (size_t i = 0; i < plate_count; i++)
    {
        plates[i].vtx_incidence = Eigen::MatrixXi::Constant(plate_degree(i, i), plates[i].vtx_edge.size(), 0);
        for (size_t j = 0; j < plate_count; j++) {
            if (i != j) {

            }
        }
        for (size_t j = 0; j < plate_count; j++) {
            if (i != j) {
                //TODO: add movement weights to plate_laplacian
                //  sum of weights for each plate must equal zero
                if (plate_laplacian(i, j) < 0)
                {// If plates i and j are adjacent, then
                    std::cout << "plates " << i << " and " << j << " are adjacent\n";
                }
                // Creates incidence matrix (num connected plates x num connecting edges)



            }
        }
        //std::cout << "Plate " << i << " incidence matrix:\n";
        //std::cout << plates[i].vtx_incidence << std::endl;
    }
}
