#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 pos;
out vec4 view_pos;
out vec3 view_pos_normal;
out vec4 proj_light_pos;

uniform vec3 light_pos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    pos = position;
    view_pos = u_Model * vec4(pos, 1.0);
    mat4 normalmatrix = transpose(inverse(u_Model));
    view_pos_normal = (normalmatrix * vec4(normal.xyz, 1.0)).xyz;
    proj_light_pos = u_Projection * vec4(light_pos, 1); // light is straight above

    gl_Position = u_Projection * u_View * u_Model * vec4(pos, 1.0f);
};