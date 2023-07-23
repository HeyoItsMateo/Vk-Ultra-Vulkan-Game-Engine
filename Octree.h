#ifndef hOctree
#define hOctree

struct cube {
    std::vector<cube> nodes;
    std::vector<glm::vec3> vertices;
    std::vector<uint16_t> indices;
    glm::vec3 center;
    glm::vec3 dimensions;

    cube(glm::vec3 Center, glm::vec3 Dimensions, int offset = 0) {
        genVertices(Center, Dimensions);
        genIndices();

        if (offset != 0) {
            offsetIndices(offset);
        }
    }
    
private:
    void genVertices(glm::vec3 center, glm::vec3 dimensions) {
        vertices.resize(8);
        this->center = center;
        this->dimensions = dimensions;

        float size = 1;
        std::vector<glm::vec3> metrics = { { size,  size,  size },
                                           { size,  size, -size },
                                           { size, -size, -size },
                                           { size, -size,  size } };
        for (int i = 0; i < 8; i++) {
            if (i < 4) {
                vertices[i] = center + (metrics[i] * dimensions);
            }
            else {
                metrics[i - 4] *= glm::vec3(-1, 1, 1);
                vertices[i] = center + (metrics[i - 4] * dimensions);
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
        int temp0 = 4;
        int temp1 = 0;
        for (int x = 16; x < 24; x += 2) {
            indices[x] = temp0;
            indices[x + 1] = temp1;

            temp0++;
            temp1++;
        }
    }
    void offsetIndices(int offset) {
        for (auto& index : indices) {
            index += offset;
        }
    }
};

struct Octree: VectorBuffer<glm::vec3>, VectorBuffer<uint16_t> {
    std::vector<cube> Nodes;
    std::vector<glm::vec3> Bounds;
    std::vector<uint16_t> Indices;
	float minSize;
	glm::vec3 center;

    std::vector<glm::mat4> matrices;

	Octree(std::vector<Vertex>& vertices, float minNodeSize) {
		minSize = minNodeSize;
        genRoot(vertices);
        checkTreeV2(vertices);
        genResources();

        VectorBuffer<glm::vec3>::init(Bounds);
        VectorBuffer<uint16_t>::init(Indices);
        indexCount = static_cast<uint32_t>(Indices.size());
	}
    int indexCount;
    int instanceCount;
public:
    void draw(VkCommandBuffer& commandBuffer) {
        VectorBuffer<glm::vec3>::bind(commandBuffer);
        VectorBuffer<uint16_t>::bind(commandBuffer);

        vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
    }

    const static VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    static VkVertexInputBindingDescription vkCreateBindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(glm::vec3);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }
    static std::vector<VkVertexInputAttributeDescription> vkCreateAttributes() {
        std::vector<VkVertexInputAttributeDescription> Attributes{
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // Position
        };
        return Attributes;
    }
    static VkPipelineVertexInputStateCreateInfo vkCreateVertexInput() {
        static auto bindingDescription = vkCreateBindings();
        static auto attributeDescriptions = vkCreateAttributes();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        return vertexInputInfo;
    }
protected:
    glm::vec3 dimensions;
    std::vector<glm::vec3> containerCenters;
    void genRoot(std::vector<Vertex>& vertices) {
        float numVerts = 1;
        glm::vec3 sumPos(0.f);
        for (Vertex& vtx : vertices) {
            sumPos += vtx.pos;
            center = sumPos / numVerts;
            if (glm::length(vtx.pos - center) > minSize) {
                minSize = glm::length(vtx.pos - center);
            }
            numVerts += 1;
        }
        dimensions = glm::vec3(minSize);

        cube Root(center, dimensions*0.75f);
        Nodes.push_back(Root);
        instanceCount++;

        Bounds.insert(Bounds.begin(), Root.vertices.begin(), Root.vertices.end());
        Indices.insert(Indices.begin(), Root.indices.begin(), Root.indices.end());

        if ((Bounds.size() == 0) or (Indices.size() == 0)) {
            throw std::runtime_error("Failed to create octree!");
        }
    }
    
private:
    int offset = 0;
    float nLayers = 1;
    float layerCount = 1;
    void checkTreeV2(std::vector<Vertex>& vertices) {
        if (vertices.size() > 1) {
            genLayer(Nodes[0]);
            layerCount+=1;
            for (int i = 0; i < 8; i++) {
                //int count = 0;
                for (auto& vertex : vertices) {
                    float dist = glm::distance(vertex.pos, Nodes[0].nodes[i].center);
                    
                    if (0 <= glm::dot(vertex.pos, Nodes[0].nodes[i].center) <= 0.5) {
                        if (dist <= glm::length(Nodes[0].nodes[i].dimensions)) {
                            
                            genLayer(Nodes[0].nodes[i]);
                        }
                    }
                    //layerCount++;
                    //count++;
                }
            }
            //layerCount++;
        }
    }

    void genResources() {
        genResource(center, Nodes[0].dimensions);

        for (auto& node : Nodes[0].nodes) {
            genResource(node.center, node.dimensions);
        }

        
    }
    void genResource(glm::vec3 Position, glm::vec3 dimensions) {
        glm::mat4 modelMatrix(1.f);
        modelMatrix = glm::translate(glm::scale(modelMatrix, dimensions), Position);

        matrices.push_back(modelMatrix);
    }
    
    void genLayer(cube& parentNode) {
        for (int i = 0; i < 8; i++) {
            glm::vec3 childCenter = parentNode.vertices[i] / 2.f;
            glm::vec3 childDimensions = parentNode.dimensions / 4.f;
            cube child(childCenter, childDimensions, (i+1)*8 );
            parentNode.nodes.push_back(child);

            Bounds.insert(Bounds.end(), child.vertices.begin(), child.vertices.end());
            Indices.insert(Indices.end(), child.indices.begin(), child.indices.end());
        }
        
    }

    std::vector<glm::vec3> genVertices(glm::vec3 center, glm::vec3 dimensions) {
        float size = 0.5;

        std::vector<glm::vec3> vertices(8);
        std::vector<glm::vec3> metrics = { { size, size, size },
                                           { size, size,-size },
                                           { size,-size,-size },
                                           { size,-size, size } };
        for (int x = 0; x < 8; x++) {
            if (x < 4) {
                vertices[x] = center + (metrics[x] * dimensions);
            }
            else {
                metrics[x - 4] *= glm::vec3(-1, 1, 1);
                vertices[x] = center + (metrics[x - 4] * dimensions);
            }
        }
        return vertices;
    }
    std::vector<uint16_t> genIndices(int offset = 0) {
        std::vector<uint16_t> indices(24);
        indices[7] = 0;
        indices[15] = 4;
        for (int x = 0; x < 7; x++) {
            indices[x] = floor((x + 1) / 2);
            indices[x + 8] = floor((x + 1) / 2) + 4;
        }
        int temp0 = 4;
        int temp1 = 0;
        for (int x = 16; x < 24; x += 2) {
            indices[x] = temp0;
            indices[x + 1] = temp1;

            temp0++;
            temp1++;
        }
        if (offset != 0) {
            for (auto& index : indices) {
                index += offset;
            }
        }
        return indices;
    }
};

template<typename T, typename V>
inline void testFunc(T& in, V& out) {
    out = in;
}

#endif
