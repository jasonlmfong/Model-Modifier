#version 460 core

layout(location = 0) out vec4 color;

in vec3 pos;
in vec4 view_pos;
in vec3 view_pos_normal;
in vec4 proj_light_pos;

uniform vec3 light_col;

void main()
{
	vec3 ka; // ambient constant
	vec3 kd; // diffuse constant
	vec3 ks; // specular constnat
	float p; // phong exponent
	vec3 n = normalize(view_pos_normal);
	vec3 v = normalize(-view_pos.xyz);
	vec3 l = normalize(proj_light_pos.xyz - view_pos.xyz);
	vec3 h = normalize(v + l);
	vec3 I = light_col; // light intensity of light

	ka = vec3(0.2, 0.2, 0.2);
	kd = vec3(0.8, 0.8, 0.8);
	ks = vec3(0.1, 0.1, 0.1);
	p = 500.0f;
	
	color = vec4(ka * 0.2 + kd * I * max(0.0, dot(n, l)) + ks * I * pow(max(0, dot(n, h)), p), 1.0);
};