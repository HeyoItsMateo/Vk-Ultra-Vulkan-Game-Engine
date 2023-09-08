#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 fragNormal;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 fragTexCoord;

struct camera{
    mat4 view;
    mat4 proj;
    vec3 position;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    double dt;
    mat4 model;
    camera cam; 
} ubo;

void main() {
    gl_Position = ubo.cam.proj * ubo.cam.view * inPosition;
    fragColor = inColor;
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}