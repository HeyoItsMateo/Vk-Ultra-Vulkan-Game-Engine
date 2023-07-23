#version 450

layout(location = 0) in vec3 inPosition;

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
}

/*
void main() {
    gl_Position = UBO.cam.proj * UBO.cam.view * SSBO.model[gl_InstanceIndex]* vec4(inPosition, 1.0);
}
*/