#include "Shader.h"

#include "glad/glad.h"
#include <fstream>
#include <iostream>

Shader::Shader(const std::string &vertexFilepath, const std::string &fragmentFilepath)
{
    std::string vertexSource, fragmentSource;

    // Read vertex shader source code from file
    {
        std::string src;
        std::string temp;
        std::ifstream fs;
        fs.open(vertexFilepath, std::fstream::in);

        while (std::getline(fs, temp))
        {
            src += temp + '\n';
        }

        fs.close();
        vertexSource = src;
    }

    // Read fragment shader source code from file
    {
        std::string src;
        std::string temp;
        std::ifstream fs;
        fs.open(fragmentFilepath, std::fstream::in);

        while (std::getline(fs, temp))
        {
            src += temp + '\n';
        }

        fs.close();
        fragmentSource = src;
    }

    const GLchar *vertexSourceCstr = vertexSource.c_str();
    const GLchar *fragmentSourceCstr = fragmentSource.c_str();

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    {
        glShaderSource(vertexShader, 1, &vertexSourceCstr, NULL);
        glCompileShader(vertexShader);

        GLint isCompiled = GL_FALSE;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
        if (!isCompiled)
        {
            GLint logLength = 0;
            glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);
            GLchar *message = (GLchar *)malloc(logLength * sizeof(GLchar));
            glGetShaderInfoLog(vertexShader, logLength, &logLength, message);

            std::cout << "Vertex shader compilation failed: " << message << std::endl;
            glDeleteShader(vertexShader);
        }
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    {
        glShaderSource(fragmentShader, 1, &fragmentSourceCstr, NULL);
        glCompileShader(fragmentShader);

        GLint isCompiled = GL_FALSE;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
        if (!isCompiled)
        {
            GLint logLength = 0;
            glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
            GLchar *message = (GLchar *)malloc(logLength * sizeof(GLchar));
            glGetShaderInfoLog(fragmentShader, logLength, &logLength, message);

            std::cout << "Fragment shader compilation failed: " << message << std::endl;
            glDeleteShader(fragmentShader);
        }
    }

    // Link shader program
    m_ID = glCreateProgram();

    glAttachShader(m_ID, vertexShader);
    glAttachShader(m_ID, fragmentShader);
    glLinkProgram(m_ID);
    glValidateProgram(m_ID);

    GLint isLinked;
    glGetProgramiv(m_ID, GL_LINK_STATUS, &isLinked);
    if (isLinked != GL_TRUE)
    {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetProgramInfoLog(m_ID, 1024, &log_length, message);

        std::cout << "Shader linking failed: " << message << std::endl;
        glDeleteProgram(m_ID);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(m_ID);
}

void Shader::Bind() const
{
    glUseProgram(m_ID);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

GLuint Shader::GetID() const
{
    return m_ID;
}

void Shader::SetUniform1i(const std::string& name, const int &value) const
{
    GLint uniformLocation = GetUniformLocation(name);
    glUniform1i(uniformLocation, value);
}

void Shader::SetUniform1iv(const std::string& name, const int count, const int* values) const
{
    GLint uniformLocation = GetUniformLocation(name);
    glUniform1iv(uniformLocation, count, values);
}

void Shader::SetUniform1f(const std::string& name, const float f) const
{
    GLint uniformLocation = GetUniformLocation(name);
    glUniform1f(uniformLocation, f);
}

void Shader::SetUniform3f(const std::string& name, const float v0, const float v1, const float v2) const
{
    GLint uniformLocation = GetUniformLocation(name);
    glUniform3f(uniformLocation, v0, v1, v2);
}

void Shader::SetUniform3fv(const std::string& name, const int count, const float* values) const
{
    GLint uniformLocation = GetUniformLocation(name);
    glUniform3fv(uniformLocation, count, values);
}

void Shader::SetUniformMat4f(const std::string &name, const glm::mat4 &value) const
{
    GLint uniformLocation = GetUniformLocation(name);
    glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &value[0][0]);
}

GLint Shader::GetUniformLocation(const std::string &name) const
{
    return glGetUniformLocation(m_ID, name.c_str());
}
