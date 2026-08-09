#include "Surface.h"

#include <algorithm>
#include <set>

namespace SurfaceDetail
{

// computer quadric matrix by summing all K_p matrices of a vertice v0
glm::mat4 ComputePlaneQuadric(const Mesh& mesh, VertexRecord v0)
{
    glm::mat4 quadric{ 0.0f };
    // for each neighbouring face, compute K_p
    glm::vec3 position = v0.position;
    for (unsigned int faceIdx : v0.adjFacesIdx)
    {
        FaceRecord face = mesh.m_TopoFaces[faceIdx];
        glm::vec3 pos0 = mesh.m_TopoVertices[face.verticesIdx[0]].position;
        glm::vec3 pos1 = mesh.m_TopoVertices[face.verticesIdx[1]].position;
        glm::vec3 pos2 = mesh.m_TopoVertices[face.verticesIdx[2]].position;
        glm::vec3 faceNormal = glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
        glm::vec4 plane{ faceNormal, -glm::dot(faceNormal, position) }; // plane equation ax+by+cz+d = 0

        quadric += glm::outerProduct(plane, plane); // K_p
    }
    return quadric;
}

// build the matrix for solving optimal vertex position
glm::mat4 BuildQuadricSolverMatrix(const glm::mat4& Quad)
{
    glm::mat4 MatQ;
    MatQ[0][0] = Quad[0][0]; MatQ[1][0] = Quad[0][1]; MatQ[2][0] = Quad[0][2]; MatQ[3][0] = Quad[0][3];
    MatQ[0][1] = Quad[0][1]; MatQ[1][1] = Quad[1][1]; MatQ[2][1] = Quad[1][2]; MatQ[3][1] = Quad[1][3];
    MatQ[0][2] = Quad[0][2]; MatQ[1][2] = Quad[1][2]; MatQ[2][2] = Quad[2][2]; MatQ[3][2] = Quad[2][3];
    MatQ[0][3] = 0;          MatQ[1][3] = 0;          MatQ[2][3] = 0;          MatQ[3][3] = 1;
    return MatQ;
}

// compute the optimal vertex position and error for a valid pair contraction
void ComputeOptimalVertexAndError(const Mesh& mesh, ValidPair& validPair, const glm::mat4& quadric1, const glm::mat4& quadric2)
{
    glm::mat4 Quad = quadric1 + quadric2;
    glm::mat4 MatQ = BuildQuadricSolverMatrix(Quad);

    if (glm::determinant(MatQ) != 0)
    {
        glm::vec4 newVPos = glm::inverse(MatQ) * glm::vec4{ 0, 0, 0, 1 };

        // calculate error: v^T * Q * v
        validPair.error = glm::dot(newVPos, Quad * newVPos);

        // change back to 3d coords from homogeneous coordinates
        validPair.newVert = glm::vec3(newVPos) / newVPos.w;
    }
    else
    {
        glm::vec4 end1 = { mesh.m_TopoVertices[validPair.vertOne].position, 1.0f };
        float end1Error = glm::dot(end1, Quad * end1);

        glm::vec4 end2 = { mesh.m_TopoVertices[validPair.vertTwo].position, 1.0f };
        float end2Error = glm::dot(end2, Quad * end2);

        glm::vec4 mid = (end1 + end2) / 2.0f;
        float midError = glm::dot(mid, Quad * mid);

        float minError = std::min({ end1Error, end2Error, midError });
        validPair.error = minError;
        if (minError == end1Error)
        {
            validPair.newVert = glm::vec3(end1) / end1.w;
        }
        else if (minError == end2Error)
        {
            validPair.newVert = glm::vec3(end2) / end2.w;
        }
        else
        {
            validPair.newVert = glm::vec3(mid) / mid.w;
        }
    }
}

// update adjacency indices after face removal, removing deleted indices and shifting remaining ones
void UpdateAdjacencyIndices(std::vector<unsigned int>& adjFaces, const std::vector<unsigned int>& removedFaceIndices)
{
    std::vector<unsigned int> newAdjFaces;
    for (unsigned int faceIdx : adjFaces)
    {
        // count how many removed faces have indices less than this one
        unsigned int offset = 0;
        for (unsigned int removedIdx : removedFaceIndices)
        {
            if (removedIdx < faceIdx)
            {
                offset++;
            }
            else if (removedIdx == faceIdx)
            {
                // this face was removed, skip it
                offset = UINT_MAX;
                break;
            }
        }

        if (offset != UINT_MAX)
        {
            newAdjFaces.push_back(faceIdx - offset);
        }
    }
    adjFaces = newAdjFaces;
}

// Shared output builder for both QEM and LineQEM
void WriteTopologyToMesh(Mesh& mesh)
{
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<unsigned int, unsigned int> NumberPolygons;

    for (VertexRecord& vert : mesh.m_TopoVertices)
    {
        mesh.GetVertIndex(vert.position, VertexPos, VertLookup);
    }

    for (FaceRecord& face : mesh.m_TopoFaces)
    {
        std::vector<unsigned int> vertsIdx;

        for (unsigned int i = 0; i < face.verticesIdx.size(); i++)
        {
            if (face.verticesIdx[i] < mesh.m_TopoVertices.size())
            {
                glm::vec3 vert = mesh.m_TopoVertices[face.verticesIdx[i]].position;
                vertsIdx.push_back(mesh.GetVertIndex(vert, VertexPos, VertLookup));
            }
        }

        // Only add face if it has at least 3 unique vertices
        std::set<unsigned int> uniqueVerts(vertsIdx.begin(), vertsIdx.end());
        if (uniqueVerts.size() >= 3)
        {
            FaceIndices.push_back(vertsIdx);
            NumberPolygons[static_cast<unsigned int>(vertsIdx.size())] += 1;
        }
    }

    mesh.DestroyTopology();
    mesh.DestroyRenderData();
    mesh.m_VertexPos = std::move(VertexPos);
    mesh.m_FaceIndices = std::move(FaceIndices);
    mesh.m_NumPolygons = std::move(NumberPolygons);
    mesh.TriangulateFaces();
}

} // namespace SurfaceDetail
