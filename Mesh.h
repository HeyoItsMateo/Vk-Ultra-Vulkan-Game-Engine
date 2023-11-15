#ifndef hMesh
#define hMesh

#include "vk.buffers.h"
#include "vk.primitives.h"

namespace vk {
    struct Mesh {
        alignas (16) glm::mat4 matrix = glm::mat4(1.f);
        template <typename T, typename U>
        inline Mesh(std::vector<T> vertices, std::vector<U> indices) 
            : indexCount(setIndexCount(indices)),
            VBO(vertices.size() * sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
            EBO(indices.size() * sizeof(U), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
            stageVBO(vertices.data(), VBO.size),
            stageEBO(indices.data(), EBO.size)
        {           
            stageVBO.transferData(VBO.buffer);
            stageEBO.transferData(EBO.buffer);
        }
        ~Mesh() = default;
    public:
        Buffer VBO, EBO;
        template<typename T>
        inline void update(std::vector<T>& vertices, std::vector<uint16_t>& indices) {
            stageVBO.update(vertices, VBO.buffer);
            stageEBO.update(indices, EBO.buffer);
        }
        virtual void draw(uint32_t instanceCount = 1) {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VBO.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, EBO.buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
        }
    protected:
        uint32_t indexCount;
        StageBuffer stageVBO, stageEBO;
        template<typename T, typename U>
        inline void subdivide(std::vector<T>& vertices, std::vector<U>& indices) {
            VBO.~Buffer();
            EBO.~Buffer();
            stageVBO.~StageBuffer();
            stageEBO.~StageBuffer();

            indexCount = static_cast<uint32_t>(indices.size() * 3);

            VBO = Buffer(vertices.size() * sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            EBO = Buffer(indices.size() * sizeof(U), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            stageVBO = StageBuffer(vertices.data(), VBO.size);
            stageEBO = StageBuffer(indices.data(), EBO.size);
        }
    private:
        template<typename U>
        inline uint32_t setIndexCount(std::vector<U> indices) {
            if (sizeof(U) == 6) {
                return indices.size() * 3;
            }
            return indices.size();
        }
    };

    struct test_Mesh {
        alignas (16) glm::mat4 matrix = glm::mat4(1.f);
        template <typename T, typename U>
        inline test_Mesh(std::vector<T> vertices, std::vector<U> indices)
            : indexCount(setIndexCount(indices))
        {
            VBO = new Buffer(vertices.size() * sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            EBO = new Buffer(indices.size() * sizeof(U), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            stageVBO = new StageBuffer(vertices.data(), VBO->size);
            stageEBO = new StageBuffer(indices.data(), EBO->size);
            stageVBO->transferData(VBO->buffer);
            stageEBO->transferData(EBO->buffer);
        }
        ~test_Mesh() {
            delete VBO;
            delete EBO;
            delete stageVBO;
            delete stageEBO;
        };
    public:
        Buffer* VBO;
        Buffer* EBO;
        template<typename T, typename U>
        inline void update(std::vector<T>& vertices, std::vector<U>& indices) {
            stageVBO->update(vertices.data(), VBO->buffer);
            stageEBO->update(indices.data(), EBO->buffer);
        }
        virtual void draw(uint32_t instanceCount = 1) {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(*VBO).buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, EBO->buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
        }
    protected:
        uint32_t indexCount;
        StageBuffer* stageVBO;
        StageBuffer* stageEBO;
        template<typename T, typename U>
        inline void subdivide(std::vector<T>& vertices, std::vector<U>& indices) {
            delete VBO;
            delete EBO;
            delete stageVBO;
            delete stageEBO;

            indexCount = static_cast<uint32_t>(indices.size() * 3);

            VBO = new Buffer(vertices.size() * sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            EBO = new Buffer(indices.size() * sizeof(U), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            stageVBO = new StageBuffer(vertices.data(), VBO->size);
            stageEBO = new StageBuffer(indices.data(), EBO->size);

            stageVBO->transferData(VBO->buffer);
            stageEBO->transferData(EBO->buffer);
        }
    private:
        template<typename U>
        inline uint32_t setIndexCount(std::vector<U> indices) {
            if (sizeof(U) == 6) {
                return indices.size() * 3;
            }
            return indices.size();
        }
    };

    struct Collider
    {//TODO: create collision mesh
        // Options to include: default cube, sphere, or low-poly mesh
        //  Goal: reduce interacting vertices but maintain object form
    };

    struct hashGrid : Mesh {
        hashGrid(glm::vec3 dimensions, glm::vec3 resolution)
            : Mesh(createVertices(dimensions, resolution), createIndices(dimensions))
        {

        }
    private:
        std::vector<lineList> createVertices(glm::vec3 dimensions, glm::vec3 resolution);
        std::vector<uint16_t> createIndices(glm::vec3 dimensions);
    };
}

#endif