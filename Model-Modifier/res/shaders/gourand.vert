#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 total_color;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

vec3 view_pos;
vec3 view_pos_normal;

uniform vec3 light_pos[3];
uniform vec3 light_col[3]; // light color
uniform int light_toggled[3]; // light on/off

uniform vec3 ambient; // ambient constant
uniform vec3 diffuse; // diffuse constant
uniform vec3 specular; // specular constnat
uniform float shine; // phong exponent

vec3 compute_light(vec3 lightpos, vec3 lightcol)
{
	vec3 n = normalize(view_pos_normal);
	vec3 v = normalize(-view_pos);
	vec3 l = normalize(lightpos - view_pos);
	vec3 h = normalize(v + l);
	
	vec3 diffuse_light = diffuse * lightcol * max(0.0, dot(n, l));
	vec3 spec_light = specular * lightcol * pow(max(0, dot(n, h)), shine);
	return diffuse_light + spec_light;
}

void main()
{
    vec4 view_pos4 = u_View * u_Model * vec4(position, 1.0);
    vec3 view_pos = view_pos4.xyz / view_pos4.w;

    mat3 normalmatrix = transpose(inverse(mat3(u_Model)));
    view_pos_normal = normalmatrix * normal;

    gl_Position = u_Projection * view_pos4;

	total_color = ambient * 0.2;
	for (int i = 0; i < 3; i++)
    {
		if (light_toggled[i] == 1)
			total_color += compute_light(light_pos[i], light_col[i]);
    }
};