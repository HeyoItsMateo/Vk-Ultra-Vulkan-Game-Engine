#version 450

layout(location = 0) in vec4 fragNormal;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 fragTexCoord;

layout(set = 1, binding = 1) uniform sampler2D chartSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(chartSampler, fragTexCoord);
}