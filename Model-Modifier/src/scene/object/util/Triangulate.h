#pragma once

#include <vector>
#include <limits>
#include "../../../external/glm/ext/vector_float2.hpp"
#include "../../../external/glm/ext/vector_float3.hpp"
#include "../../../external/glm/geometric.hpp"


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

std::vector<std::vector<int>> TriangulatePolygonalFace(std::vector<unsigned int> faceIdx, std::vector<glm::vec3> all_vertices)
{
    // poly is a list of indices into m_VertexPos
    // where each item in vertpos is a glm::vec3
    std::vector<glm::vec3> poly_vertices;
    for (unsigned int corner : faceIdx)
    {
        poly_vertices.push_back(all_vertices[corner]);
    }

    glm::vec3 origin = poly_vertices[0];
    glm::vec3 basis1 = glm::normalize(poly_vertices[1] - origin);
    glm::vec3 basis2 = glm::normalize(poly_vertices[2] - origin);
    // get plane normal and represent all vertices as points on this new plane (xy coords)
    glm::vec3 face_normal = glm::normalize(glm::cross(basis1, basis2));

    // build orthogonal basis
    basis2 = glm::cross(face_normal, basis1);

    // represent all vertices as a linear combination of the 2d bases
    std::vector<glm::vec2> vertices_on_plane;
    for (glm::vec3 point : poly_vertices)
    {
        glm::vec3 dist_from_origin = point - origin;
        double u_coord = glm::dot(basis1, dist_from_origin);
        double v_coord = glm::dot(basis2, dist_from_origin);
        vertices_on_plane.push_back({ u_coord, v_coord });
    }

    int n = vertices_on_plane.size();
    // perform minimum cost triangulation calculation
    std::vector<std::vector<int>> best_vertex = mct(vertices_on_plane, n);

    // get triangulation
    std::vector<std::vector<int>> triangulation = extract_triangulation(best_vertex, 0, n - 1);
    return triangulation;
}
