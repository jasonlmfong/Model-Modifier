#pragma once

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexBufferLayout;

class VertexArray
{
private:
	unsigned int m_ID;
public:
	VertexArray();
	~VertexArray();

	void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);

	void Bind() const;
	void Unbind() const;
};