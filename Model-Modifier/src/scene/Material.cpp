#include "Material.h"

Material::Material()
{
	m_Ambient = {0.2f, 0.2f, 0.2f};
	m_Diffuse = {0.0f, 0.55f, 0.15f};
	m_Specular = {0.72f, 1.0f, 0.68f};
	m_Shine = 50.0f;
}

Material::~Material()
{
}
