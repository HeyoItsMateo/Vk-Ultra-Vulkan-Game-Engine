#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

struct camera{
    mat4 view;
    mat4 proj;
    vec3 position;
};

layout(set = 1, binding = 0) readonly buffer inSSBO {
   mat4 model[];
} SSBO;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    float dt;
    mat4 model;
    camera cam; 
} UBO;

void main() {
    gl_Position = UBO.cam.proj * UBO.cam.view * UBO.model* vec4(inPosition, 1.0);
    fragColor = vec4(inColor,1.f);
    fragTexCoord = inTexCoord;
}

/*
void main() {
    gl_Position = UBO.cam.proj * UBO.cam.view * SSBO.model[gl_InstanceIndex]* vec4(inPosition, 1.0);
    fragColor = vec4(inColor,1.f);
    fragTexCoord = inTexCoord;
}
*/