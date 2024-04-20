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

vec3 compute_light(vec3 lightpos, vec3 lightcol)
{
	vec3 n = normalize(view_pos_normal);
	vec3 v = normalize(-view_pos);
	vec3 l = normalize(lightpos - view_pos);
	vec3 h = normalize(v + l);
	
	float diffuse = max(0.0, dot(n, l));
	float spec = max(0, dot(n, h));
	float intensity = 0.6 * diffuse + 0.4 * spec;

 	if (intensity > 0.9) {
 		intensity = 1.1;
 	}
 	else if (intensity > 0.5) {
 		intensity = 0.7;
 	}
 	else {
 		intensity = 0.5;
	}

	vec3 diffuse_light = diffuse * lightcol * 0.6 * intensity;
	
	vec3 spec_light = specular * lightcol * pow(0.4 * intensity, shine);
	return diffuse_light + spec_light;
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