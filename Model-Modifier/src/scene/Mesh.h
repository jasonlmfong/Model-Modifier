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

enum shading
{
	FLAT,
	SMOOTH
};

class Mesh
{
public:
	Mesh(const char* filename);
	~Mesh();

	void loadOBJ(const char* filename);
	void Destroy();
	void Reload(const char* filename);

	void preRender(int shading);

public:
	std::vector<glm::vec3> m_VertexPos;
	std::vector<std::vector<unsigned int>> m_FaceIndices;
	std::vector<glm::vec3> m_FaceNormals;

	unsigned int m_FlatNumVert;
	float* m_FlatVertices;
	unsigned int m_FlatNumIdx;
	unsigned int* m_FlatIndices;

	std::vector<glm::vec3> m_SmoothVertexNormals;
	unsigned int m_SmoothNumVert;
	float* m_SmoothVertices;
	unsigned int m_SmoothNumIdx;
	unsigned int* m_SmoothIndices;

	unsigned int m_OutNumVert;
	float* m_OutVertices;
	unsigned int m_OutNumIdx;
	unsigned int* m_OutIndices;
};
