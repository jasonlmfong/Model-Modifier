#include "PlaneProjection.h"


std::vector<glm::vec2> projectPolygonToPlane(std::vector<glm::vec3> polygonVertices)
{
    glm::vec3 origin = polygonVertices[0];
    glm::vec3 basis1 = glm::normalize(polygonVertices[1] - origin);
    glm::vec3 basis2 = glm::normalize(polygonVertices[2] - origin);
    // get plane normal and represent all vertices as points on this new plane (xy coords)
    glm::vec3 face_normal = glm::normalize(glm::cross(basis1, basis2));

    // build orthogonal basis
    basis2 = glm::cross(face_normal, basis1);

    // represent all vertices as a linear combination of the 2D bases
    std::vector<glm::vec2> vertices_on_plane;
    for (glm::vec3 point : polygonVertices)
    {
        glm::vec3 dist_from_origin = point - origin;
        double u_coord = glm::dot(basis1, dist_from_origin);
        double v_coord = glm::dot(basis2, dist_from_origin);
        vertices_on_plane.push_back({ u_coord, v_coord });
    }

    return vertices_on_plane;
}
