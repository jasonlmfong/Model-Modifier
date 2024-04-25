#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "../../external/glm/ext/vector_float2.hpp"


// order vertices by counter clockwise angle
std::vector<unsigned int> OrderPolygonVertices(std::vector<glm::vec2> planePolygonVertices, glm::vec2 centroid, unsigned int numVerts);
