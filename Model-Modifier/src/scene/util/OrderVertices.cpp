#include "OrderVertices.h"

float getAngle(glm::vec2 vert, glm::vec2 centroid)
{
	return std::atan2(vert.y - centroid.y, vert.x - centroid.x);
}


// order vertices by counter clockwise angle
std::vector<unsigned int> OrderPolygonVertices(std::vector<glm::vec2> planePolygonVertices, glm::vec2 centroid, unsigned int numVerts)
{
	std::vector<std::pair<float, unsigned int>> angles_order(numVerts);
	for (unsigned int i = 0; i < numVerts; i++)
	{
		angles_order[i] = { getAngle(planePolygonVertices[i], centroid), i };
	}

	// sort by angles in ascending order
	std::sort(angles_order.begin(), angles_order.end());

	std::vector<unsigned int> vertexOrder(numVerts);
	for (unsigned int j = 0; j < numVerts; j++) {
		vertexOrder[j] = angles_order[j].second;
	}
	return vertexOrder;
}
