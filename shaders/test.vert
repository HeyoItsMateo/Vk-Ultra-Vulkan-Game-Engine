#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {

    gl_PointSize = 14.0;
    gl_Position = vec4(inPosition.xyz, 1.0);
    fragColor = inColor.rgba;
}