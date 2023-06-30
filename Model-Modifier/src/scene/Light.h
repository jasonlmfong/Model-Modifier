#pragma once

#include "../external/glm/ext/vector_float3.hpp"

#include "../external/glm/ext/vector_float4.hpp"
#include "../external/glm/geometric.hpp"

class Light
{
public:
	Light(glm::vec3 pos, glm::vec3 col);
	~Light();

public:
	float* m_Pos;
	float* m_Col;
};