#include "Triangulate.h"

double triCost(glm::vec2 a, glm::vec2 b, glm::vec2 c)
{
    return glm::distance(a, b) + glm::distance(b, c) + glm::distance(c, a);
}

// minimum cost triangulation
std::vector<std::vector<unsigned int>> mct(std::vector<glm::vec2> points, unsigned int size)
{
    std::vector<std::vector<double>> costMemo(size, std::vector<double>(size));
    std::vector<std::vector<unsigned int>> bestVertex(size, std::vector<unsigned int>(size));

    for (unsigned int gap = 0; gap < size; gap++)
    {
        for (unsigned int i = 0, j = gap; j < size; i++, j++)
        {
            if (j < i + 2)
                costMemo[i][j] = 0.0;
            else
            {
                costMemo[i][j] = std::numeric_limits<double>::infinity();
                for (unsigned int k = i + 1; k < j; k++)
                {
                    double val = costMemo[i][k] + costMemo[k][j] + triCost(points[i], points[j], points[k]);
                    if (costMemo[i][j] > val)
                    {
                        costMemo[i][j] = val;
                        bestVertex[i][j] = k;
                    }
                }
            }
        }
    }
    return bestVertex;
}

std::vector<std::vector<unsigned int>> extractTriangulation(std::vector<std::vector<unsigned int>> bestVertex, unsigned int i, unsigned int j)
{
    std::vector<std::vector<unsigned int>> triangulation;

    unsigned int best = bestVertex[i][j];
    if (j == i + 1)
    {
        // base case: i and j are adjacent, no triangulation needed
        return triangulation;
    }

    // {i, best, j} form a valid triangle
    std::vector<std::vector<unsigned int>> leftTriangulation = extractTriangulation(bestVertex, i, best);
    triangulation.insert(triangulation.end(), leftTriangulation.begin(), leftTriangulation.end());

    triangulation.push_back({ i, best, j });

    std::vector<std::vector<unsigned int>> rightTriangulation = extractTriangulation(bestVertex, best, j);
    triangulation.insert(triangulation.end(), rightTriangulation.begin(), rightTriangulation.end());

    return triangulation;
}

// can assume faceIdx.size() > 3
std::vector<std::vector<unsigned int>> triangulatePolygonalFace(std::vector<unsigned int> faceIdx, std::vector<glm::vec3> allVertices)
{
    std::vector<glm::vec3> polyVertices;
    for (unsigned int corner : faceIdx)
    {
        polyVertices.push_back(allVertices[corner]);
    }
    std::vector<glm::vec2> verticesOnPlane = projectPolygonToPlane(polyVertices);

    unsigned int n = static_cast<unsigned int>(verticesOnPlane.size());
    // perform minimum cost triangulation calculation
    std::vector<std::vector<unsigned int>> best_vertex = mct(verticesOnPlane, n);

    // get triangulation
    std::vector<std::vector<unsigned int>> triangulation = extractTriangulation(best_vertex, 0, n - 1);
    return triangulation;
}
