#include "Material.h"

Material::Material()
{
	m_Ambient = new float[3] {0.2, 0.2, 0.2};
	m_Diffuse = new float[3] {0.8, 0.8, 0.8};
	m_Specular = new float[3] {0.1, 0.1, 0.1};
	m_Shine = 500.0f;
}

Material::~Material()
{
}
