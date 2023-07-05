#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 fragColor;

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


void main() {
    gl_PointSize = 14.0;
    gl_Position = ubo.cam.proj * ubo.cam.view * vec4(inPosition, 1.0);
    fragColor = vec4(inColor,1.f);
}