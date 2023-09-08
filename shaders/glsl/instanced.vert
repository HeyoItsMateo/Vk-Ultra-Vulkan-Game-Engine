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

layout(set = 1, binding = 0) readonly buffer inSSBO {
   mat4 model[];
} iSSBO;

layout(set = 1, binding = 1) buffer outSSBO {
   mat4 model[];
} oSSBO;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    double dt;
    mat4 model;
    camera cam; 
} UBO;

void main() {
    oSSBO.model[gl_InstanceIndex] = iSSBO.model[gl_InstanceIndex];

    gl_Position = UBO.cam.proj * UBO.cam.view * oSSBO.model[gl_InstanceIndex] * inPosition;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}