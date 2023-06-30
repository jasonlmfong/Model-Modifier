#pragma once

#include <string>

#include "glad/glad.h"
#include "../external/glm/glm.hpp"

class Shader
{
public:
	Shader(const std::string &vertexFilepath, const std::string &fragmentFilepath);
	~Shader();

	void Bind() const;
	void Unbind() const;

	GLuint GetID() const;

	//Set uniforms
	void SetUniform1i(const std::string& name, const int& value) const;
	void SetUniformMat4f(const std::string &name, const glm::mat4 &matrix) const;
	void SetUniform3f(const std::string& name, const float v0, const float v1, const float v2) const;
private:

	GLint GetUniformLocation(const std::string &name) const;

private:
	GLuint m_ID;
};