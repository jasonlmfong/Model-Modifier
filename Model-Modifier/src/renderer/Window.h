#pragma once

#include "glad/glad.h"
#include "glfw/glfw3.h"

class Window
{
private:
	GLFWwindow *m_ID;

public:
	Window(unsigned int width, unsigned int height, const char *title, bool isFullscreen);
	~Window();

	GLFWwindow *GetID() const;

private:
	int Create(unsigned int width, unsigned int height, const char *title, bool isFullscreen);
	void Destroy();
};