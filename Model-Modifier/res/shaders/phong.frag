#version 460 core

layout(location = 0) out vec4 color;

in vec3 pos;
in vec4 view_pos;
in vec3 view_pos_normal;
in vec4 proj_light_pos;

uniform vec3 light_col; // light color
uniform vec3 ambient; // ambient constant
uniform vec3 diffuse; // diffuse constant
uniform vec3 specular; // specular constnat
uniform float shine; // phong exponent

void main()
{
	vec3 ka = ambient;
	vec3 kd = diffuse;
	vec3 ks = specular;
	float p = shine;
	vec3 n = normalize(view_pos_normal);
	vec3 v = normalize(-view_pos.xyz);
	vec3 l = normalize(proj_light_pos.xyz - view_pos.xyz);
	vec3 h = normalize(v + l);
	vec3 I = light_col; // light intensity of light
	
	color = vec4(ka * 0.2 + kd * I * max(0.0, dot(n, l)) + ks * I * pow(max(0, dot(n, h)), p), 1.0);
};