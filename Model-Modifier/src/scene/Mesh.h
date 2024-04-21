#pragma once

#include <vector>
#include <unordered_map>

#include "../external/glm/ext/vector_float3.hpp"
#include "../external/glm/geometric.hpp"

#include "object/Object.h"

enum shading
{
	FLAT,
	MIXED,
	SMOOTH
};

class Mesh
{
public:
	Mesh(Object obj, int shading);
	~Mesh();

	void BuildFaceNormals();

	void BuildVerticesIndices();
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
