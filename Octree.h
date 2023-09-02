#ifndef hOctree
#define hOctree

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>
#include <bitset>

std::unordered_map<glm::vec4, std::bitset<8>> octreeMap = {
            { { 1, 1, 1, 1}, 0b10000000 },
            { {-1, 1, 1, 1}, 0b01000000 },
            { {-1,-1, 1, 1}, 0b00100000 },
            { { 1,-1, 1, 1}, 0b00010000 },

            { { 1, 1,-1, 1}, 0b00001000 },
            { {-1, 1,-1, 1}, 0b00000100 },
            { {-1,-1,-1, 1}, 0b00000010 },
            { { 1,-1,-1, 1}, 0b00000001 },

            { { 0, 0, 0, 1}, 0b00000000 },
            { { 1, 0, 0, 1}, 0b10011001 },
            { {-1, 0, 0, 1}, 0b01100110 },
            { { 0, 1, 0, 1}, 0b11001100 },
            { { 0,-1, 0, 1}, 0b00110011 },
            { { 0, 0, 1, 1}, 0b11110000 },
            { { 0, 0,-1, 1}, 0b00001111 },
            
            
            { { 0, 0,-1, 1}, 0b00001111 },
            { { 0, 1, 1, 1}, 0b11000000 },
            { { 0,-1, 1, 1}, 0b00110000 },
            { { 1, 0, 1, 1}, 0b10010000 },
            { {-1, 0, 1, 1}, 0b01100000 },
};

struct Voxel : Vertex {
    std::vector<Voxel> nodes;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    glm::vec4 center;
    glm::vec4 dimensions{1.f};
    glm::mat4 matrix;
    Voxel(glm::vec4 Center, glm::vec4 Dimensions)
        : center(Center), dimensions(Dimensions)
    {
        genVertices();
        genIndices();
        matrix = genMatrix(Center, Dimensions);
    }
public:
    bool containsVertex = false;
    const static VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    template <typename T>
    inline void checkVoxel(std::vector<T>& vertices) {
        for (auto& vertex : vertices) {
            float dist = glm::distance(vertex.position, center);
            float dotp = glm::dot(center, vertex.position);

            if ((0.5f <= dotp <= 1.5f) and (dist <= glm::length(dimensions))) {
                containsVertex = true;
                break;
            }
        }
    }
private:
    void genVertices() {
        vertices.resize(8);
        std::vector<glm::vec4> metrics = { { 1,  1,  1,  1 },
                                           { 1,  1, -1,  1 },
                                           { 1, -1, -1,  1 },
                                           { 1, -1,  1,  1 } };
        for (int i = 0; i < 8; i++) {
            if (i < 4) {
                vertices[i].position = metrics[i];
                vertices[i].color = glm::vec4(glm::vec3(0.85f), 1.f);
                //vertices[i].texCoord = glm::vec2(0.f);
            }
            else {
                metrics[i - 4] *= glm::vec4(-1, 1, 1, 1);
                vertices[i].position = metrics[i - 4];
                vertices[i].color = glm::vec4(glm::vec3(0.85f), 1.f);
                //vertices[i].texCoord = glm::vec2(0.f);
            }
        }
    }
    void genIndices() {
        indices.resize(24);

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
    }
    glm::mat4 genMatrix(glm::vec3 Center, glm::vec3 Dimensions) {
        glm::mat4 _matrix(1.f);
        return glm::scale(glm::translate(_matrix, Center), Dimensions);
    }
};

template <typename T>
struct Octree : vk::GameObject {
    std::vector<Voxel> Nodes;
    std::vector<glm::mat4> matrices;
    //int maxInstances = 128;
    int maxDepth = 8;
    Octree(std::vector<T>& vertices, float minSize, float rootScale = 0.8f) {
        init_Root(vertices, minSize, rootScale);

        checkTree(vertices, Nodes[0], currentDepth-1);

        init_GameObject(Nodes[0].vertices, Nodes[0].indices);
    }
private:
    int currentDepth = 0;
    void init_Root(std::vector<T>& vertices, float minSize, float rootScale)
    {// Generates the dimensions of the root node and creates the root node.
        float numVerts = 1;
        glm::vec4 center(0.f), sumPos(0.f);
        for (Vertex& vtx : vertices) {
            sumPos += vtx.position;
            center = sumPos / numVerts;
            if (glm::distance(vtx.position, center) > minSize) {
                minSize = glm::distance(vtx.position, center);
            }
            numVerts += 1;
        }
        Voxel Root(center, glm::vec4(glm::vec3(minSize * rootScale), 1.f));
        Nodes.push_back(Root);
        if ((Nodes[0].vertices.size() == 0) or (Nodes[0].indices.size() == 0)) {
            throw std::runtime_error("Failed to create root!");
        }
        matrices.push_back(Root.matrix);
        currentDepth++;
    }    
    void checkTree(std::vector<T>& vertices, Voxel& Node, int depth) {
        Node.checkVoxel(vertices);
        if (Node.containsVertex) {
            for (auto& vertex : Node.vertices) {
                Voxel child = genChild(vertex, Node);
                if (depth < maxDepth) {
                    checkTree(vertices, child, depth + 1);
                }
                else if (depth <= 0) {
                    return;
                }
                currentDepth = depth - 1;
            }           
        }
    }
    Voxel genChild(T& vertex, Voxel& Node) {
        glm::vec4 center = Node.matrix * glm::vec4(glm::vec3(vertex.position * 0.5f), 1.f);
        Voxel child(center, 0.5f * Node.dimensions);
        matrices.push_back(child.matrix);
        Node.nodes.push_back(child);
        instanceCount++;
        return child;
    }

    inline static std::unordered_map<glm::vec4, std::bitset<8>> map = {
            { { 1, 1, 1, 1}, 0b10000000 },
            { {-1, 1, 1, 1}, 0b01000000 },
            { {-1,-1, 1, 1}, 0b00100000 },
            { { 1,-1, 1, 1}, 0b00010000 },

            { { 1, 1,-1, 1}, 0b00001000 },
            { {-1, 1,-1, 1}, 0b00000100 },
            { {-1,-1,-1, 1}, 0b00000010 },
            { { 1,-1,-1, 1}, 0b00000001 },

            { { 0, 0, 0, 1}, 0b00000000 },
            { { 1, 0, 0, 1}, 0b10011001 },
            { {-1, 0, 0, 1}, 0b01100110 },
            { { 0, 1, 0, 1}, 0b11001100 },
            { { 0,-1, 0, 1}, 0b00110011 },
            { { 0, 0, 1, 1}, 0b11110000 },
            { { 0, 0,-1, 1}, 0b00001111 },

            { { 1, 1, 0, 1}, 0b10001000 },
            { {-1, 1, 0, 1}, 0b01000100 },
            { {-1,-1, 0, 1}, 0b00100010 },
            { { 1,-1, 0, 1}, 0b00010001 },

            { { 0, 1, 1, 1}, 0b11000000 },
            { { 0,-1, 1, 1}, 0b00110000 },
            { { 0, 1,-1, 1}, 0b00001100 },
            { { 0,-1,-1, 1}, 0b00000011 },

            { { 1, 0, 1, 1}, 0b10010000 },
            { {-1, 0, 1, 1}, 0b01100000 },
            { {-1, 0,-1, 1}, 0b00000110 },
            { { 1, 0,-1, 1}, 0b00001001 },
    };
};

template <typename T>
struct biOctree {
    std::bitset<8> node;
    std::array<biOctree, 8> leaves;

    glm::vec4 center{ 0.f };
    float dimensions;

    inline biOctree(std::vector<T>& vertices) {
        init_Root(vertices);
        for (auto& vertex : vertices) {
            if (checkVertex(vertex)) {

            }
        }
        for (int i = 0; i < node.size(); i++) {
            if (node[i]) {

            }
        }
    }
    bool checkVertex(T& vertex) {
        float phi, theta;
        // TODO: 
        //   solve vertex position for phi and theta, then
        //   use phi and theta to find quadrant location. finally,
        //   update bitset to reflect vertex containing quadrant.
        // TODO:
        //   or convert vertex coordinates to octree coordinates
        //   and find the octant that way. Probably faster imo.
        float dist = glm::distance(vertex.position, center);
        float dotp = glm::dot(center, vertex.position);

        if ((0.5 <= dotp <= 1.5) and (dist <= dimensions)) {
            return true;
        }
        return false;
    }

    void init_Root(std::vector<T>& vertices, float minSize, float rootScale)
    {// Generates the dimensions of the root node and creates the root node.
        float numVerts = 1;
        glm::vec4 sumPos(0.f);
        for (Vertex& vtx : vertices) {
            sumPos += vtx.position;
            center = sumPos / numVerts;
            if (glm::distance(vtx.position, center) > minSize) {
                minSize = glm::distance(vtx.position, center);
            }
            numVerts += 1;
        }
        dimensions = minSize * rootScale;
    }
};

std::unordered_map<glm::vec4, std::bitset<4>> quadtreeMap = {
    { { 1, 1, 0, 1},{0b0001} },
    { {-1, 1, 0, 1},{0b0010} },
    { { 1,-1, 0, 1},{0b0100} },
    { {-1,-1, 0, 1},{0b1000} },
    { { 0, 0, 0, 1},{0b0000} }
};

#endif
