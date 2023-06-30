#pragma once

#include "VertexBuffer.h"

class IndexBuffer
{
private:
	unsigned int m_ID;
	unsigned int m_Count;

public:
	IndexBuffer(const void* data, unsigned int count, DRAW_MODE mode);
	~IndexBuffer();

	void AssignData(const void* data, unsigned int count, DRAW_MODE mode);

	void Bind() const;
	void Unbind() const;

	unsigned int GetCount() const;
};