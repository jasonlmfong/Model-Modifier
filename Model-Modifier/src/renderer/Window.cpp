#include "Window.h"

#include <iostream>

Window::Window(unsigned int width, unsigned int height, const char *title, bool isFullscreen)
    : m_ID(nullptr)
{
    int result = Create(width, height, title, isFullscreen);
    if (result == -1)
    {
        std::cout << "Failed to instantiate window object." << std::endl;
        Destroy();
    }
}

Window::~Window()
{
    Destroy();
}


GLFWwindow *Window::GetID() const
{
    return m_ID;
}

int Window::Create(unsigned int width, unsigned int height, const char *title, bool isFullscreen)
{
    /* Initialize the library */
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    m_ID = glfwCreateWindow(width, height, title, isFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!m_ID)
    {
        std::cout << "Failed to create window." << std::endl;
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(m_ID);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context." << std::endl;
        return -1;
    }

    glViewport(0, 0, width, height);

    return 0;
}

void Window::Destroy()
{
    glfwTerminate();
}

