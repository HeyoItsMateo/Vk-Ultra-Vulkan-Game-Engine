#ifndef hScene
#define hScene

#include "Mesh.h"

namespace vk {
	struct Scene {
        Scene(Pipeline& renderPipeline, Mesh& gameObject)
            : pPipeline(&renderPipeline), pGameObject(&gameObject) {}
    public:
        void render() {
            pPipeline->bind();
            pGameObject->draw();
        }
    private:
        Pipeline* pPipeline;
        Mesh* pGameObject;
	};
}

#endif