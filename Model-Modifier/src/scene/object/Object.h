#pragma once

#include <iostream>
#include <fstream>
#include <ios>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "../../external/glm/ext/vector_float3.hpp"
#include "../../external/glm/geometric.hpp"

class Object
{
public:
	Object();
	Object(const char* filename);
	~Object();

	void loadOBJ(const char* filename);
	void Rescale();
	void Destroy();
	void Reload(const char* filename);

	std::vector<std::vector<unsigned int>>  TriangulateFaces(std::vector<std::vector<unsigned int>> faces);

public:
	glm::vec3 m_Min;
	glm::vec3 m_Max;

	std::vector<glm::vec3> m_VertexPos;
	std::vector<std::vector<unsigned int>> m_FaceIndices;

	std::unordered_map<int, int> m_NumPolygons;
};
