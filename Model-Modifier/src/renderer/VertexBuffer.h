#pragma once

enum class DRAW_MODE {
	STATIC, DYNAMIC
};

class VertexBuffer
{
private:
	unsigned int m_ID;

public:
	VertexBuffer(const void *data, unsigned int size, DRAW_MODE mode);
	~VertexBuffer();

	void AssignData(const void* data, unsigned int size, DRAW_MODE mode) const;

	void Bind() const;
	void Unbind() const;
};