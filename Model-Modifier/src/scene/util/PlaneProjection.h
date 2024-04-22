#pragma once

#include <vector>
#include "../../external/glm/ext/vector_float2.hpp"
#include "../../external/glm/ext/vector_float3.hpp"
#include "../../external/glm/geometric.hpp"


std::vector<glm::vec2> projectPolygonToPlane(std::vector<glm::vec3> polygonVertices);
