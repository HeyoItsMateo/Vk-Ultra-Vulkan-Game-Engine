#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 vertNormal;
layout(location = 1) out vec4 vertColor;
layout(location = 2) out vec2 vertTexCoord;

void main()
{
	vertNormal = inNormal;
	gl_Position = vec4(inPosition.xyz, 1.0);
}