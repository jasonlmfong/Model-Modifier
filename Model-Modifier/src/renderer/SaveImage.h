#pragma once

#include <string>

#include "../external/stb/stb_image_write.h"

#include "glad/glad.h"
#include "glfw/glfw3.h"


void saveImage(const std::string& filepath, GLFWwindow* window)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    GLsizei stride = 3 * width;
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    GLsizei bufferSize = stride * height;
    std::vector<char> buffer(bufferSize);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filepath.c_str(), width, height, 3, buffer.data(), stride);
}
