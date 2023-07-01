#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 normal_vec;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    normal_vec = normal;

    gl_Position = u_Projection * u_View * u_Model * vec4(position, 1.0f);
};