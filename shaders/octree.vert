#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

struct camera{
    mat4 view;
    mat4 proj;
    vec3 position;
};

layout(set = 2, binding = 0) readonly buffer inSSBO {
   mat4 model[];
} iSSBO;

layout(set = 2, binding = 1) buffer outSSBO {
   mat4 model[];
} oSSBO;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    float dt;
    mat4 model;
    camera cam; 
} UBO;

void main() {
    oSSBO.model[gl_InstanceIndex] = iSSBO.model[gl_InstanceIndex];

    gl_Position = UBO.cam.proj * UBO.cam.view * oSSBO.model[gl_InstanceIndex] * inPosition;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

/*
void main() {
    gl_Position = UBO.cam.proj * UBO.cam.view * SSBO.model[gl_InstanceIndex]* vec4(inPosition, 1.0);
    fragColor = vec4(inColor,1.f);
    fragTexCoord = inTexCoord;
}
*/