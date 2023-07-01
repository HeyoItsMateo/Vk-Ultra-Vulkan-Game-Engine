#version 450

struct camera{
    mat4 view;
    mat4 proj;
};

struct modelMatrix {
    mat4 model;
};


layout(set = 0, binding = 0) uniform UniformBufferObject {
    modelMatrix model;
    camera cam;
    float dt;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.cam.proj * ubo.cam.view * ubo.model.model * vec4(inPosition, 1.0);
    fragColor = vec4(inColor,1.f);
    fragTexCoord = inTexCoord;
}