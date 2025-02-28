#pragma once

#include <iostream>
#include <fstream>
#include <ios>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "../../external/glm/ext/vector_float3.hpp"
#include "../../external/glm/geometric.hpp"
#include "../util/Triangulate.h"

class Object
{
public:
	Object();
	Object(const std::string &filename);
	~Object();

	void loadOBJ(const std::string &filename);
	void Rescale();
	void Destroy();
	void Reload(const std::string &filename);

	void TriangulateFaces();

	void MakeTriangleMesh();

public:
	glm::vec3 m_Min;
	glm::vec3 m_Max;

	std::vector<glm::vec3> m_VertexPos;
	std::vector<std::vector<unsigned int>> m_FaceIndices;
	std::vector<std::vector<unsigned int>> m_TriFaceIndices;

	std::unordered_map<unsigned int, unsigned int> m_NumPolygons;
};
