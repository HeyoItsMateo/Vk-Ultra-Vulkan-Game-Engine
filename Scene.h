#ifndef hScene
#define hScene

#include "Mesh.h"

namespace vk {
	struct Scene {
        Scene(Pipeline& renderPipeline, test_Mesh& gameObject)
            : pPipeline(&renderPipeline), pGameObject(&gameObject)
        {}
    public:
        void render(VkCommandBuffer& commandBuffer) {
            pPipeline->bind();
            pGameObject->draw(commandBuffer);
        }
    private:
        Pipeline* pPipeline;
        test_Mesh* pGameObject;
	};
}

#endif