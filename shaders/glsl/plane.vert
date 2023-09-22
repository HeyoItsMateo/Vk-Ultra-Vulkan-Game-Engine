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

layout(set = 1, binding = 0) uniform model_UBO{
    mat4 matrix;
} plane;

layout(set = 2, binding = 0) uniform sampler imageSampler;

layout (set = 3, binding = 0) uniform readonly image2D heightMap;

void main() {
    vec4 heightData = imageLoad(heightMap, ivec2(inPosition.xz));
    vec4 pos = vec4(inPosition.x, length(heightData), inPosition.z, inPosition[3]);

    gl_Position = ubo.cam.proj * ubo.cam.view * plane.matrix * pos;
    fragColor = inColor;
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}