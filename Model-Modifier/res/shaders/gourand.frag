#version 460 core

layout(location = 0) out vec4 color;

in vec3 total_color;

void main()
{
	color = vec4(total_color, 1.0);
};