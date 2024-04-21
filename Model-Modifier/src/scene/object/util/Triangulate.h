#pragma once

#include <vector>
#include <limits>
#include "../../../external/glm/ext/vector_float2.hpp"
#include "../../../external/glm/ext/vector_float3.hpp"
#include "../../../external/glm/geometric.hpp"

#include "PlaneProjection.h"

double tri_cost(glm::vec2 a, glm::vec2 b, glm::vec2 c)
{
    return glm::distance(a, b) + glm::distance(b, c) + glm::distance(c, a);
}

// minimum cost triangulation
std::vector<std::vector<int>> mct(std::vector<glm::vec2> points, int size)
{
    std::vector<std::vector<double>> cost_memo(size, std::vector<double>(size));
    std::vector<std::vector<int>> best_vertex(size, std::vector<int>(size));

    for (int gap = 0; gap < size; gap++)
    {
        for (int i = 0, j = gap; j < size; i++, j++)
        {
            if (j < i + 2)
                cost_memo[i][j] = 0.0;
            else
            {
                cost_memo[i][j] = std::numeric_limits<double>::infinity();
                for (int k = i + 1; k < j; k++)
                {
                    double val = cost_memo[i][k] + cost_memo[k][j] + tri_cost(points[i], points[j], points[k]);
                    if (cost_memo[i][j] > val)
                    {
                        cost_memo[i][j] = val;
                        best_vertex[i][j] = k;
                    }
                }
            }
        }
    }
    return best_vertex;
}

std::vector<std::vector<int>> extract_triangulation(std::vector<std::vector<int>> best_vertex, int i, int j)
{
    std::vector<std::vector<int>> triangulation;

    int best = best_vertex[i][j];
    if (j == i + 1)
    {
        // base case: i and j are adjacent, no triangulation needed
        return triangulation;
    }

    // {i, best, j} form a valid triangle
    std::vector<std::vector<int>> leftTriangulation = extract_triangulation(best_vertex, i, best);
    triangulation.insert(triangulation.end(), leftTriangulation.begin(), leftTriangulation.end());

    triangulation.push_back({ i, best, j });

    std::vector<std::vector<int>> rightTriangulation = extract_triangulation(best_vertex, best, j);
    triangulation.insert(triangulation.end(), rightTriangulation.begin(), rightTriangulation.end());

    return triangulation;
}

// can assume faceIdx.size() > 3
std::vector<std::vector<int>> TriangulatePolygonalFace(std::vector<unsigned int> faceIdx, std::vector<glm::vec3> all_vertices)
{
    std::vector<glm::vec2> vertices_on_plane = projectPolygonToPlane(faceIdx, all_vertices);

    int n = static_cast<int>(vertices_on_plane.size());
    // perform minimum cost triangulation calculation
    std::vector<std::vector<int>> best_vertex = mct(vertices_on_plane, n);

    // get triangulation
    std::vector<std::vector<int>> triangulation = extract_triangulation(best_vertex, 0, n - 1);
    return triangulation;
}
