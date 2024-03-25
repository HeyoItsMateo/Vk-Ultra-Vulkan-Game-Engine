#include "Planet.h"
#include "ProcGenLib.h"

Plate::Plate(float radius)
{
    std::thread t_pos([this, radius] { init_pos(radius, -1.f, 1.f, radius*0.15f); });
    std::jthread jt_rgb([this] { init_rgb(0.f, 1.f, 0.1f); });

    t_pos.join();
    std::jthread jt_rot([this] { init_rot(-1.f, 1.f); });
}
void Plate::init_pos(float radius, float min, float max, float offset)
{
    std::uniform_real_distribution<float> XYZ(min, max);
    glm::vec3 pos{
        XYZ(pgl::global_rng),
        XYZ(pgl::global_rng),
        XYZ(pgl::global_rng)
    };

    std::uniform_real_distribution<float> _offset(-offset, offset);
    position = (radius + _offset(pgl::global_rng)) * glm::normalize(pos);
}
void Plate::init_rgb(float min, float max, float offset)
{
    std::uniform_real_distribution<float> RGB(min, max);
    glm::vec3 colour{
        RGB(pgl::global_rng),
        RGB(pgl::global_rng),
        RGB(pgl::global_rng)
    };

    std::uniform_real_distribution<float> _offset(-offset, offset);
    color = { _offset(pgl::global_rng) + glm::normalize(colour), 1.f };
}
void Plate::init_rot(float min, float max) {
    std::uniform_real_distribution<float> XYZ(min, max);
    glm::vec3 randOrtho{
        XYZ(pgl::global_rng),
        XYZ(pgl::global_rng),
        XYZ(pgl::global_rng)
    };
    axis = glm::normalize(glm::cross(randOrtho, position));

    std::uniform_real_distribution<float> angularVelocity(-0.02f, 0.02f);
    omega = angularVelocity(pgl::global_rng);
}
