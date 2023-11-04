#ifndef hScene
#define hScene

#include "Mesh.h"

namespace vk {
	struct Scene {
        Scene(Pipeline& renderPipeline, test_Mesh& gameObject)
            : pPipeline(&renderPipeline), pGameObject(&gameObject) {}
    public:
        void render() {
            pPipeline->bind();
            pGameObject->draw();
        }
    private:
        Pipeline* pPipeline;
        test_Mesh* pGameObject;
	};
}

#endif