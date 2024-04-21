#version 460 core

layout(location = 0) out vec4 color;

in vec3 view_pos;
in vec3 view_pos_normal;

uniform vec3 light_pos[3];
uniform vec3 light_col[3]; // light color
uniform int light_toggled[3]; // light on/off

uniform vec3 ambient; // ambient constant
uniform vec3 diffuse; // diffuse constant
uniform vec3 specular; // specular constant
uniform float shine; // phong exponent

uniform float metallic; // metalic constant
uniform float roughness; // roughness constant

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

float geometrySmith(float n_dot_v, float n_dot_l)
{
    float k = (roughness * roughness) / 8;
    float ggx1 = n_dot_v / (n_dot_v * (1.0 - k) + k);
    float ggx2 = n_dot_l / (n_dot_l * (1.0 - k) + k);
    return ggx1 * ggx2;
}

float distribution(float n_dot_h)
{
    float r4 = pow(roughness, 4.0);
    float denom = PI * pow((pow(n_dot_h, 2.0) * (r4 - 1.0) + 1.0), 2.0);
    return r4 / denom;
}

vec3 compute_light(vec3 lightpos, vec3 lightcol)
{
    vec3 n = normalize(view_pos_normal);
    vec3 v = normalize(-view_pos);
    vec3 l = normalize(lightpos - view_pos);
    vec3 h = normalize(v + l);

    float n_dot_v = max(dot(n, v), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float n_dot_h = max(dot(n, h), 0.0);
    
    vec3 F = fresnelSchlick(max(dot(h, v), 0.0), vec3(0.04));
    float G = geometrySmith(n_dot_v, n_dot_l);
    float D = distribution(n_dot_h);

    // Cook-Torrance BRDF
    vec3 spec_light = specular * (D * G * F) / (4.0 * n_dot_v * n_dot_l + 0.001);

    // Combine with diffuse
    vec3 kS = F; // Fresnel reflectance
    vec3 kD = vec3(1.0) - kS; // Diffuse reflectance
    kD *= 1.0 - metallic; // Metallic materials have no diffuse reflection

    return (kD * diffuse / PI + spec_light) * n_dot_l * lightcol;
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