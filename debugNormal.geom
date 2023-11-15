#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in vec4 inNormal[];
layout(location = 1) in vec4 vertColor[];
layout(location = 2) in vec2 vertTexCoord[];

layout(location = 0) out vec4 geomColor;

struct camera{
    mat4 view;
    mat4 proj;
    vec3 position;
};

layout(set = 0, binding = 1) uniform UniformBufferObject {
    double dt;
    mat4 model;
    camera cam; 
} ubo;

layout(set = 1, binding = 0) uniform model_UBO{
    mat4 matrix;
} ico;


void main(void)
{	
	float normalLength = 0.02;
	for(int i=0; i<gl_in.length(); i++)
	{
		vec3 pos = gl_in[i].gl_Position.xyz;
		vec3 normal = inNormal[i].xyz;

		gl_Position = ubo.cam.proj * ubo.cam.view * (ico.matrix * vec4(pos, 1.0));
		geomColor = vec3(1.0, 0.0, 0.0);
		EmitVertex();

		gl_Position = ubo.cam.proj * ubo.cam.view * (ico.matrix * vec4(pos + normal * normalLength, 1.0));
		geomColor = vec3(0.0, 0.0, 1.0);
		EmitVertex();

		EndPrimitive();
	}
}