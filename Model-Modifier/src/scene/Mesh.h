#pragma once

#include <vector>

#include "../external/glm/ext/vector_float3.hpp"
#include "../external/glm/geometric.hpp"

#include "Object.h"

enum shading
{
	FLAT,
	SMOOTH
};

class Mesh
{
public:
	Mesh(Object obj, int shading);
	~Mesh();

	void BuildVerticesIndices();
	void BuildVerticesIndices(Object obj);
	void BuildVerticesIndices(int shading);
	void Destroy();
	void Rebuild();
	void Rebuild(Object obj);
	void Rebuild(int shading);

public:
	Object m_Object;
	int m_ShadingType;

	std::vector<glm::vec3> m_FaceNormals;

	unsigned int m_OutNumVert;
	float* m_OutVertices;
	unsigned int m_OutNumIdx;
	unsigned int* m_OutIndices;
};
