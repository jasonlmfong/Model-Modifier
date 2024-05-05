#pragma once

#include "GLFW/glfw3.h"

class Input
{
public:
	static void Init(GLFWwindow* window);

	// Input polling
	static bool IsKeyDown(int keycode);
	static bool IsMouseButtonDown(int button);

	static float GetScrollX();
	static float GetScrollY();

	static void ResetScroll();

private:
	// Input events
	static void SetCallbacks();
	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	
	static GLFWwindow* m_WindowID;
	static float m_ScrollX;
	static float m_ScrollY;
};