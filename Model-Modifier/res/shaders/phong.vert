#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 pos;
out vec3 view_pos;
out vec3 view_pos_normal;
out vec3 proj_light_pos[3];

uniform vec3 light_pos[3];

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    vec4 view_pos4 = u_View * u_Model * vec4(position, 1.0);
    view_pos = view_pos4.xyz / view_pos4.w;

    mat3 normalmatrix = transpose(inverse(mat3(u_View * u_Model)));
    view_pos_normal = normalmatrix * normal;
    
    for (int i = 0; i < 3; i++)
    {
        proj_light_pos[i] = (u_Projection * vec4(light_pos[i], 1)).xyz;
    }

    gl_Position = u_Projection * view_pos4;
};