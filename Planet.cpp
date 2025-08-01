#include "Planet.h"

pgl::Plate::Plate(float radius) {
    // Initialize random distributions for generating coordinate vectors
    std::uniform_real_distribution<float> XYZ(-1.0f, 1.0f);
    std::uniform_real_distribution<float> offset(-0.25f, 0.25f);

    // Initialize random position of plate center
    std::thread t_pos([this, radius, &XYZ, &offset] { position = radius * generate_random_vector3(XYZ, offset); });

    // Initialize random vector for use in generating the axis of rotation
    glm::vec3 randOrtho(0.f);
    std::thread t_ortho([this, &randOrtho, &XYZ, &offset] { randOrtho = generate_random_vector3(XYZ, offset); });

    // Initialize random plate color
    std::jthread t_rgb([this] { color = generate_random_color4(0.1f, 1.0f); });

    // Initialize random angular velocity
    std::uniform_real_distribution<float> angularVelocity(-0.02f, 0.02f);
    omega = angularVelocity(global_rng);

    // Generate axis of rotation and movement vector.
    t_ortho.join();
    t_pos.join();
    axis = glm::normalize(glm::cross(randOrtho, position));
    // movDir = glm::normalize(glm::cross(axis, position));

    //plateID = i;
}

glm::vec3 pgl::Plate::generate_random_vector3(std::uniform_real_distribution<float>& xyz_distribution, std::uniform_real_distribution<float>& offset) {
    glm::vec3 vector3{
        xyz_distribution(global_rng) + offset(global_rng),
        xyz_distribution(global_rng) + offset(global_rng),
        xyz_distribution(global_rng) + offset(global_rng)
    };
    return glm::normalize(vector3);

}