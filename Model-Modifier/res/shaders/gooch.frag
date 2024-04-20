#version 460 core

layout(location = 0) out vec4 color;

in vec3 view_pos;
in vec3 view_pos_normal;

uniform vec3 light_pos[3];
uniform vec3 light_col[3]; // light color
uniform int light_toggled[3]; // light on/off

uniform vec3 ambient; // ambient constant
uniform vec3 diffuse; // diffuse constant
uniform vec3 specular; // specular constnat
uniform float shine; // phong exponent

uniform vec3 warm;
uniform vec3 cool;
uniform float alpha;
uniform float beta;

vec3 compute_light(vec3 lightpos, vec3 lightcol)
{
	vec3 n = normalize(view_pos_normal);
	vec3 v = normalize(-view_pos);
	vec3 l = normalize(lightpos - view_pos);
	vec3 h = normalize(v + l);

	vec3 k_warm = min(warm + alpha * diffuse, 1.0);
	vec3 k_cool = min(cool + beta * diffuse, 1.0);

	float gooch = (1.0 + dot(l, n)) / 2.0;
	vec3 gooch_diffuse = gooch * k_warm +  (1 - gooch) * k_cool;
	
	vec3 spec_light = specular * lightcol * pow(max(0, dot(n, h)), shine);
	return gooch_diffuse + spec_light;
}


void main()
{
	vec3 total_light = ambient * 0.2;
	for (int i = 0; i < 3; i++)
	{
		if (light_toggled[i] == 1)
			total_light += compute_light(light_pos[i], light_col[i]);
	}
	color = vec4(total_light, 1.0);
};