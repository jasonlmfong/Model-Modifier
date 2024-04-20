#include "Material.h"

Material::Material()
{
	m_Ambient = new float[3] {0.2f, 0.2f, 0.2f};
	m_Diffuse = new float[3] {0.0f, 0.55f, 0.15f};
	m_Specular = new float[3] {0.72f, 1.0f, 0.68f};
	m_Shine = 50.0f;
}

Material::~Material()
{
	delete[] m_Ambient;
	delete[] m_Diffuse;
	delete[] m_Specular;
}
