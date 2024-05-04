#include "Light.h"

Light::Light()
{
	m_LightsToggled = new bool[3] { true, false, false };

	m_Pos = {
		-10, 5, 0, // top left
		10, 5, 0, // top right
		0, -10, 0 // bottom
	};

	m_Col = {
		0.498f, 0.522f, 0.333f, // olive
		1.0f, 0.466f, 1.0f, // pink
		0.259f, 0.522f, 0.967f // blue
	};

	m_Brightness = {
		1.0f,
		2.0f,
		1.5f
	};
}

Light::~Light()
{
	delete[] m_LightsToggled;
}
