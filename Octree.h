#ifndef hOctree
#define hOctree

#include <glm/gtx/string_cast.hpp>

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

            if ((0.5 <= dotp <= 1.5) and (dist <= glm::length(dimensions))) {
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
            indices[x] = floor((x + 1) / 2);
            indices[x + 8] = floor((x + 1) / 2) + 4;
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
struct Octree : GameObject {
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
};

#endif
