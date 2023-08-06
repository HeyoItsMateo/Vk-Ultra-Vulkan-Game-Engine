#ifndef hOctree
#define hOctree

struct Cube {
    std::vector<Cube> nodes;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    glm::vec3 center;
    glm::vec3 dimensions;

    Cube(glm::vec3 Center, glm::vec3 Dimensions, int offset = 0) {
        genVertices(Center, Dimensions);
        genIndices();

        if (offset != 0) {
            offsetIndices(offset);
        }
    }
public:
    const static VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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
                vertices[i].pos = center + (metrics[i] * dimensions);
                vertices[i].color = glm::vec3(1.f);
                vertices[i].texCoord = glm::vec2(0.f);
            }
            else {
                metrics[i - 4] *= glm::vec3(-1, 1, 1);
                vertices[i].pos = center + (metrics[i - 4] * dimensions);
                vertices[i].color = glm::vec3(1.f);
                vertices[i].texCoord = glm::vec2(0.f);
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

struct Octree: GameObject {
    std::vector<Cube> Nodes;
    std::vector<Vertex> Bounds;
    std::vector<uint16_t> Indices;
	float minSize;
	glm::vec3 center;

    std::vector<glm::mat4> matrices;

	Octree(std::vector<Vertex>& vertices, float minNodeSize) {
		minSize = minNodeSize;
        genRoot(vertices);
        checkTreeV2(vertices);
        genResources();

        VectorBuffer<Vertex>::init(Bounds);
        VectorBuffer<uint16_t>::init(Indices);
        indexCount = static_cast<uint32_t>(Indices.size());

        instanceCount = Nodes[0].nodes.size()+1;
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

        Cube Root(center, dimensions*0.75f);
        genResource(Root.center, Root.dimensions);

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
        for (auto& node : Nodes[0].nodes) {
            genResource(node.center, node.dimensions);
        }
    }
    void genResource(glm::vec3 Position, glm::vec3 dimensions) {
        glm::mat4 modelMatrix(1.f);
        modelMatrix = glm::translate(glm::scale(modelMatrix, dimensions), Position);

        matrices.push_back(modelMatrix);
    }
    void genLayer(Cube& parentNode) {
        for (int i = 0; i < 8; i++) {
            glm::vec3 childCenter = parentNode.vertices[i].pos / 1.f;
            glm::vec3 childDimensions = parentNode.dimensions / 2.f;
            Cube child(childCenter, childDimensions, (i+1)*8 );
            parentNode.nodes.push_back(child);

            //Bounds.insert(Bounds.end(), child.vertices.begin(), child.vertices.end());
            //Indices.insert(Indices.end(), child.indices.begin(), child.indices.end());
        }
        
    }
};


#endif
