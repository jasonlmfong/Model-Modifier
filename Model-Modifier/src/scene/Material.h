#pragma once

#include<vector>

#include "../external/glm/ext/vector_float3.hpp"
#include "../external/glm/geometric.hpp"

class Material
{
public:
	Material();
	~Material();

public:
	std::vector<float> m_Ambient;
	std::vector<float> m_Diffuse;
	std::vector<float> m_Specular;
	float m_Shine;
};