#pragma once

#include <OreRenderer/Mesh.h>

// Subdivision algorithms - modify mesh geometry in-place
void SubdivideBeehive(Mesh& mesh);
void SubdivideSnowflake(Mesh& mesh);
void SubdivideCatmullClark(Mesh& mesh);
void SubdivideDooSabin(Mesh& mesh);
void SubdivideLoop(Mesh& mesh);

// Simplification algorithms - reduce mesh to desiredCount triangles
void SimplifyGarlandHeckbert(Mesh& mesh, unsigned int desiredCount);
void SimplifyLiuRahimzadehZordan(Mesh& mesh, unsigned int desiredCount, float alpha = 0.5f);

// Shared QEM helpers (used by both simplification algorithms)
namespace SurfaceDetail
{
    glm::mat4 ComputePlaneQuadric(const Mesh& mesh, VertexRecord v0);
    glm::mat4 BuildQuadricSolverMatrix(const glm::mat4& Quad);
    void ComputeOptimalVertexAndError(const Mesh& mesh, ValidPair& validPair, const glm::mat4& quadric1, const glm::mat4& quadric2);
    void UpdateAdjacencyIndices(std::vector<unsigned int>& adjFaces, const std::vector<unsigned int>& removedFaceIndices);
    void WriteTopologyToMesh(Mesh& mesh);
}
