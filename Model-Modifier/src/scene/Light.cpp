#include "Light.h"

Light::Light(glm::vec3 pos, glm::vec3 col)
{
	m_Pos = new float[3] {pos.x, pos.y, pos.z};
	m_Col = new float[3] {col.x, col.y, col.z};
}

Light::~Light()
{
}
