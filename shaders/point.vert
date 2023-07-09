#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

struct modelMatrix{
    mat4 model;
};
struct camMatrix {
    mat4 view;
    mat4 proj;
};


layout (binding = 0) uniform ParameterUBO {
    modelMatrix model;
    camMatrix camera;
    float deltaTime;
} ubo;

void main() {

    gl_PointSize = 14.0;
    gl_Position = ubo.camera.view*ubo.camera.proj*vec4(inPosition, 1.0);
    fragColor = inColor.rgb;
}