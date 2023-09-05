#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

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
    float testDist = distance(ubo.cam.position, inPosition.xyz);
    gl_PointSize = 2 / (testDist*testDist + 0.01f);
    gl_Position = ubo.cam.proj * ubo.cam.view * inPosition;
    fragColor = vec4(inColor.rgb, 1.f);
}