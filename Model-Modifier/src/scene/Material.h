#pragma once

#include "../external/glm/ext/vector_float3.hpp"
#include "../external/glm/geometric.hpp"

class Material
{
public:
	Material();
	~Material();

public:
	float* m_Ambient;
	float* m_Diffuse;
	float* m_Specular;
	float m_Shine;
};