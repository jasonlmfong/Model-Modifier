#version 460 core

layout(location = 0) out vec4 color;

in vec3 normal_vec;

void main()
{
	color = vec4(normal_vec, 1.0);
};