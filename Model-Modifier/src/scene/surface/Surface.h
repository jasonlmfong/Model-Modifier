#include <vector>
#include <unordered_map>

#include "../../external/glm/ext/vector_float3.hpp"
#include "../../external/glm/ext/vector_uint2.hpp"
#include "../../external/glm/geometric.hpp"

#include "../Object.h"

struct VertexRecord
{
	glm::vec3 position;
	std::vector<unsigned int> adjEdgesIdx;
	std::vector<unsigned int> adjFacesIdx;
};

struct EdgeRecord
{
	unsigned endPoint1Idx; // maintain that endpoint1 < endpoint2 for searching
	unsigned endPoint2Idx;
	glm::vec3 midEdgePoint;
	std::vector<unsigned int> adjFacesIdx; // should have 1 or 2 adjacent faces
};

struct FaceRecord
{
	glm::vec3 facePoint;
	std::vector<unsigned int> verticesIdx; // each face can have 3 vertices
	std::unordered_map<unsigned int, std::vector<unsigned int>> verticesEdges; // each vertex will have 2 edges
	std::vector<unsigned int> edgesIdx; // each face can have 3 edges
};

class Surface
{
public:
	// given the position of a vertex, find index in the set inputted
	// if it does not exist, insert into our map and return
	unsigned int getVertIndex(glm::vec3 vertPos, std::vector<glm::vec3>& AllVertexPos, std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>>& VertIdxLookup);
	// given the pair of indices in m_Vertices, find index in m_Edges
	// if it does not exist, insert into our map and return
	unsigned int getEdgeIndex(glm::uvec2 vertPair);

	Surface(Object obj);
	~Surface();
	Object CCOutputOBJ(std::vector<glm::vec3> edgePoints);

	// Modification algorithms
	Object Beehive();
	Object Snowflake();
	Object CatmullClark();
	Object DooSabin();

public:
	std::vector<VertexRecord> m_Vertices;
	std::vector<EdgeRecord> m_Edges;
	std::vector<FaceRecord> m_Faces;

	glm::vec3 m_Min;
	glm::vec3 m_Max;

	std::unordered_map<unsigned int, std::unordered_map<unsigned int, unsigned int>> m_EdgeIdxLookup;
};
