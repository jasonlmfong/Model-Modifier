#pragma once

#include <vector>
#include <limits>
#include "../../external/glm/ext/vector_float2.hpp"
#include "../../external/glm/ext/vector_float3.hpp"
#include "../../external/glm/geometric.hpp"

#include "PlaneProjection.h"

// can assume faceIdx.size() > 3
std::vector<std::vector<unsigned int>> triangulatePolygonalFace(std::vector<unsigned int> faceIdx, std::vector<glm::vec3> allVertices);
