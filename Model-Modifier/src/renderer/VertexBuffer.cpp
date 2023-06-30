#include "VertexBuffer.h"

#include <iostream>
#include "glad/glad.h"

VertexBuffer::VertexBuffer(const void *data, unsigned int size, DRAW_MODE mode)
{
	glGenBuffers(1, &m_ID);
	AssignData(data, size, mode);

	int bufferSize;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if (size != bufferSize)
	{
		glDeleteBuffers(1, &m_ID);
		std::cout << "Buffer error." << std::endl;
	}
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_ID);
}

void VertexBuffer::AssignData(const void* data, unsigned int size, DRAW_MODE mode) const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_ID);
	if (mode == DRAW_MODE::STATIC)
	{
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}
	else if (mode == DRAW_MODE::DYNAMIC)
	{
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
	}
	else
	{
		glDeleteBuffers(1, &m_ID);
		std::cout << "Buffer mode error." << std::endl;
	}
}

void VertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_ID);
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}