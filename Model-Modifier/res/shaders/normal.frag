#version 460 core

layout(location = 0) out vec4 color;

in vec3 normal_vec;

void main()
{
	// normal vector can have components ranging from -1 to 1, normalize to 0 to 1 for colors
	vec3 normal = normal_vec / 2;
	normal += (0.5, 0.5, 0.5);

	color = vec4(normal, 1.0);
};