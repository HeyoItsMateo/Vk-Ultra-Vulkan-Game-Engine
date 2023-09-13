#ifndef hOctree
#define hOctree

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>
#include <bitset>
#include <tuple>
#include <map>
#include <set>

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


/* New octree method */
struct octreeNew {
    //Voxel voxel;
    float mass = 0;
    glm::vec4 CoM = {0,0,0,1};

    std::set<octreeNew> Nodes;
    std::set<octreeNew*> Leaves;

    template<typename T>
    inline octreeNew(std::vector<T>& objects)
    {
        /* Check Root Voxel for presence of objects */
        /* if more than n-allowed objects, create child nodes */
        /* and insert into Nodes set, */
        /* then calculate the node's mass and center of mass. */
        /* Rebuild the voxel's matrix around the center of mass */
        /* and scale appropriately. */

        /* Iterate through Nodes to find if children contain any objects, */
        /* and repeat until maximum depth is reached. */
        /* build child nodes centered on octants, not CoM. */
    }
    void checkNodes() {
        /* Recursively check each node from the nodes set */
        /* for if they contain any of the vectors */
        //for (auto& node : Nodes) {
        //  node.checkNodes();
        //}
    }
    template<typename T>
    inline void checkSet(std::vector<T>& objects) {
        /* second parameter for the Nodes set which checks if a vertex is contained in it */
        /* returns false if not contained */
        // for(auto& object:objects) {
        //   if(!Nodes[object]) { /* Returns true if contained */
        //      return;
        //   }
        // }
        /* External function must then check the calling node */
        /* Repeat until root is reached, if root does not contain vertex, */
        /* then rebuild the entire octree. */

        /* Could make a dark energy simulation by not rebuilding the entire octree */
        /* and resize nodes to contain any out of bounds objects, */
        /* then adjust the rest of the objects to maintain their relative locations within their node. */
    }
};

namespace vk {
    struct Octree {
        Voxel rootNode;
        std::set<Octree> subNodes;
        std::vector<glm::mat4> matrices;
        template<typename T>
        inline Octree(std::vector<T>& vertices, float minSize, float rootScale = 0.8f) {
            init_Root(vertices, minSize, rootScale);
        }
    private:
        int currentDepth = 0;
        template<typename T>
        inline void init_Root(std::vector<T>& vertices, float minSize, float rootScale)
        {// Generates the dimensions of the root node and creates the root node.
            float numVerts = 1;
            glm::vec4 center(0.f), sumPos(0.f);
            for (T& vtx : vertices) {
                sumPos += vtx.position;
                center = sumPos / numVerts;
                if (glm::distance(vtx.position, center) > minSize) {
                    minSize = glm::distance(vtx.position, center);
                }
                numVerts += 1;
            }
            rootNode(center, glm::vec4(glm::vec3(minSize * rootScale), 1.f));

            if ((rootNode.VBO.size == 0) or (rootNode.EBO.size == 0)) {
                throw std::runtime_error("Failed to create root!");
            }
            matrices.push_back(rootNode.matrix);
            currentDepth++;
        }
    };
}
/*
template <typename T>
struct Octree {
    std::vector<vk::Voxel> Nodes;
    std::vector<glm::mat4> matrices;
    std::set<vk::Voxel*> leafNodes;
    //int maxInstances = 128;
    int maxDepth = 8;
    Octree(std::vector<T>& vertices, float minSize, float rootScale = 0.8f) {
        init_Root(vertices, minSize, rootScale);

        checkTree(vertices, Nodes[0], currentDepth-1);

        init_GameObject(Nodes[0].vertices, Nodes[0].indices);
    }
    void updateTree(std::vector<T>& vertices) {
        for (auto const& node : leafNodes)
        {
            vk::Voxel temp = *node;
            temp.checkNode();
        }
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
        vk::Voxel Root(center, glm::vec4(glm::vec3(minSize * rootScale), 1.f));
        Nodes.push_back(Root);
        if ((Nodes[0].vertices.size() == 0) or (Nodes[0].indices.size() == 0)) {
            throw std::runtime_error("Failed to create root!");
        }
        matrices.push_back(Root.matrix);
        currentDepth++;
    }    
    void checkTree(std::vector<T>& vertices, vk::Voxel& Node, int depth) {
        Node.checkVoxel(vertices);
        if (Node.containsVertex) {
            for (auto& vertex : Node.vertices) {
                vk::Voxel child = genChild(vertex, Node);
                if (depth < maxDepth) {
                    checkTree(vertices, child, depth + 1);
                }
                else if (depth <= 0) {
                    leafNodes.insert(&Node);
                    return;
                }
                currentDepth = depth - 1;
            }           
        }
    }
    vk::Voxel genChild(lineList& vertex, vk::Voxel& Node) {
        glm::vec4 center = Node.matrix * glm::vec4(glm::vec3(vertex.position * 0.5f), 1.f);
        vk::Voxel child(center, 0.5f * Node.dimensions);
        child.pParent = &Node;
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
*/

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
