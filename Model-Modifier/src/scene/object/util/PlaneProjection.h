#pragma once

#include <vector>
#include "../../../external/glm/ext/vector_float2.hpp"
#include "../../../external/glm/ext/vector_float3.hpp"
#include "../../../external/glm/geometric.hpp"


std::vector<glm::vec2> projectPolygonToPlane(std::vector<unsigned int> faceIdx, std::vector<glm::vec3> all_vertices)
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

    // represent all vertices as a linear combination of the 2D bases
    std::vector<glm::vec2> vertices_on_plane;
    for (glm::vec3 point : poly_vertices)
    {
        glm::vec3 dist_from_origin = point - origin;
        double u_coord = glm::dot(basis1, dist_from_origin);
        double v_coord = glm::dot(basis2, dist_from_origin);
        vertices_on_plane.push_back({ u_coord, v_coord });
    }

    return vertices_on_plane;
}


// send polygonal face to plane, then use clockwise scheme  (from face point) to get proper face order, can start 0 index with  original 0 index

