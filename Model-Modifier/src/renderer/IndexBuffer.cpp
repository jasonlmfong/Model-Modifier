#include "IndexBuffer.h"

#include <glad/glad.h>

#include <iostream>

IndexBuffer::IndexBuffer(const void* data, unsigned int count, DRAW_MODE mode)
	: m_Count(count)
{
	glGenBuffers(1, &m_ID);
	AssignData(data, count, mode);

	int bufferSize;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if (m_Count * sizeof(unsigned int) != bufferSize)
	{
		glDeleteBuffers(1, &m_ID);
		std::cout << "Buffer error." << std::endl;
	}
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_ID);
}

void IndexBuffer::AssignData(const void* data, unsigned int count, DRAW_MODE mode)
{
	m_Count = count;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
	if (mode == DRAW_MODE::STATIC)
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Count * sizeof(unsigned int), data, GL_STATIC_DRAW);
	}
	else if (mode == DRAW_MODE::DYNAMIC)
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Count * sizeof(unsigned int), data, GL_DYNAMIC_DRAW);
	}
	else
	{
		glDeleteBuffers(1, &m_ID);
		std::cout << "Buffer mode error." << std::endl;
	}
}

void IndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
}

void IndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

unsigned int IndexBuffer::GetCount() const
{
	return m_Count;
}
