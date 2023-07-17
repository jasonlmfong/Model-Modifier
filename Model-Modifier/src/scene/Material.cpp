#include "Material.h"

Material::Material()
{
	m_Ambient = new float[3] {0.2f, 0.2f, 0.2f};
	m_Diffuse = new float[3] {0.8f, 0.8f, 0.8f};
	m_Specular = new float[3] {0.1f, 0.1f, 0.1f};
	m_Shine = 50.0f;
}

Material::~Material()
{
	delete[] m_Ambient;
	delete[] m_Diffuse;
	delete[] m_Specular;
}
