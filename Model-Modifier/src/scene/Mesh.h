#pragma once

#include <iostream>
#include <fstream>
#include <ios>
#include <sstream>
#include <vector>


#include "../external/glm/ext/vector_float3.hpp"
#include "../external/glm/geometric.hpp"

#include <glad/glad.h>
#include <glfw/glfw3.h>

class Mesh
{
public:
	Mesh(const char* filename);
	~Mesh();

	void loadOBJ(const char* filename);
	void Destroy();

public:
	std::vector<glm::vec3> m_VertexPos;
	std::vector<std::vector<unsigned int>> m_Elements;
	unsigned int* m_Indices;
	std::vector<glm::vec3> m_FaceNormals;
	std::vector<glm::vec3> m_VertexNormals;
	float* m_Vertices;
};