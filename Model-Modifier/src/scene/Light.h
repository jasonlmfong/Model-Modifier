#pragma once

#include "../external/glm/ext/vector_float3.hpp"
#include "../external/glm/geometric.hpp"

class Light
{
public:
	Light();
	~Light();

public:
	bool* m_LightsToggled;
	float* m_Pos;
	float* m_Col;
};