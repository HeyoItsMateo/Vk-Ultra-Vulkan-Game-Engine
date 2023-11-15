#version 450

layout(location = 0) in vec4 geomColor;

layout (location = 0) out vec4 outFragColor;

void main(void)
{
	outFragColor = geomColor;
}