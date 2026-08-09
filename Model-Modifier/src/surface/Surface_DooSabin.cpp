#include "Surface.h"
#include "OrderVertices.h"
#include <OreRenderer/Mesh.h>

// PlaneProjection is part of OreRenderer's internal sources
#include "scene/util/PlaneProjection.h"

////////// helpers to build the Mesh //////////

static void DSWriteToMesh(
    Mesh& mesh,
    std::vector<std::vector<glm::vec3>>& newPointsPerFace,
    std::unordered_map<unsigned int, std::vector<glm::vec3>>& pointsPerVertex,
    std::unordered_map<unsigned int, std::vector<glm::vec3>>& pointsPerEdge)
{
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<unsigned int, unsigned int> NumberPolygons;

    // new face from old face (n-gon from n-gon)
    for (unsigned int currFaceIdx = 0; currFaceIdx < mesh.m_TopoFaces.size(); currFaceIdx++)
    {
        unsigned int n = static_cast<unsigned int>(newPointsPerFace[currFaceIdx].size());
        std::vector<unsigned int> newFaceIdx;
        for (unsigned int i = 0; i < n; i++)
        {
            // use vertex lookup to avoid creating duplicate vertices
            unsigned int vertIdx = mesh.GetVertIndex(newPointsPerFace[currFaceIdx][i], VertexPos, VertLookup);
            newFaceIdx.push_back(vertIdx);
        }
        FaceIndices.push_back(newFaceIdx);
        NumberPolygons[n] += 1;
    }

    // new face from old edge (always a quad face)
    for (unsigned int currEdgeIdx = 0; currEdgeIdx < pointsPerEdge.size(); currEdgeIdx++)
    {
        EdgeRecord currEdge = mesh.m_TopoEdges[currEdgeIdx];
        // skip boundary edges, they cannot form a new face
        if (currEdge.adjFacesIdx.size() == 2)
        {
            // build neighbour face normal to match later
            glm::vec3 avgFaceNormal{ 0 };
            for (unsigned int adjFaces = 0; adjFaces < 2; adjFaces++)
            {
                avgFaceNormal += mesh.ComputeFaceNormal(mesh.m_TopoFaces[currEdge.adjFacesIdx[adjFaces]]);
            }
            avgFaceNormal /= 2.0f;

            if (pointsPerEdge[currEdgeIdx].size() == 4)
            {
                // use vertex lookup to avoid creating duplicate vertices
                unsigned int vertAIdx = mesh.GetVertIndex(pointsPerEdge[currEdgeIdx][0], VertexPos, VertLookup);
                unsigned int vertBIdx = mesh.GetVertIndex(pointsPerEdge[currEdgeIdx][1], VertexPos, VertLookup);
                unsigned int vertCIdx = mesh.GetVertIndex(pointsPerEdge[currEdgeIdx][2], VertexPos, VertLookup);
                unsigned int vertDIdx = mesh.GetVertIndex(pointsPerEdge[currEdgeIdx][3], VertexPos, VertLookup);

                // check if our normal is flipped or not
                glm::vec3 ABCNormal = mesh.ComputeFaceNormal(pointsPerEdge[currEdgeIdx][0], pointsPerEdge[currEdgeIdx][1], pointsPerEdge[currEdgeIdx][2]);
                if (glm::dot(ABCNormal, avgFaceNormal) > 0)
                {
                    // not flipped, usual triangulation of 0123
                    FaceIndices.push_back({ vertAIdx, vertBIdx, vertCIdx, vertDIdx });
                }
                else
                {
                    // flipped normals, use 3210
                    FaceIndices.push_back({ vertDIdx, vertCIdx, vertBIdx, vertAIdx });
                }
                NumberPolygons[4] += 1;
            }
        }
    }

    // new face from old vertex (n-gon for n faces the old vertex neighbours)
    for (unsigned int currVertIdx = 0; currVertIdx < mesh.m_TopoVertices.size(); currVertIdx++)
    {
        VertexRecord currVert = mesh.m_TopoVertices[currVertIdx];
        if (currVert.adjFacesIdx.size() < 3)
            continue;

        // build neighbour face normal to match later
        glm::vec3 avgFaceNormal{ 0 };
        for (unsigned int adjFaceIdx : currVert.adjFacesIdx)
        {
            avgFaceNormal += mesh.ComputeFaceNormal(mesh.m_TopoFaces[adjFaceIdx]);
        }
        avgFaceNormal /= static_cast<float>(currVert.adjFacesIdx.size());

        // get vertices for new face
        std::vector<glm::vec3> polyVertices;
        glm::vec3 centroid{ 0 };  // new facepoint
        unsigned int numNewVerts = static_cast<unsigned int>(pointsPerVertex[currVertIdx].size());
        for (unsigned int newVertIdx = 0; newVertIdx < numNewVerts; newVertIdx++)
        {
            glm::vec3 currPoint = pointsPerVertex[currVertIdx][newVertIdx];
            polyVertices.push_back(currPoint);
            centroid += currPoint;
        }
        centroid /= numNewVerts;

        // project new vertices (and facepoint) to plane
        polyVertices.push_back(centroid);
        std::vector<glm::vec2> verticesOnPlane = projectPolygonToPlane(polyVertices);
        // get the order of vertices to form the polygon
        glm::vec2 centroid2D = verticesOnPlane.back();
        verticesOnPlane.pop_back();
        std::vector<unsigned int> vertOrdering = OrderPolygonVertices(verticesOnPlane, centroid2D, numNewVerts);

        // check if our normal is flipped or not
        glm::vec3 vertFaceNormal = mesh.ComputeFaceNormal(polyVertices[vertOrdering[0]], polyVertices[vertOrdering[1]], polyVertices[vertOrdering[2]]);
        std::vector<unsigned int> newFaceIdx;
        if (glm::dot(vertFaceNormal, avgFaceNormal) > 0)
        {
            // not flipped, use the poylgon vertex ordering we have
            for (unsigned int i = 0; i < numNewVerts; i++)
            {
                // use vertex lookup to avoid creating duplicate vertices
                unsigned int vertIdx = mesh.GetVertIndex(polyVertices[vertOrdering[i]], VertexPos, VertLookup);
                newFaceIdx.push_back(vertIdx);
            }
        }
        else
        {
            // flipped normals, reverse the poylgon vertex ordering we have
            for (unsigned int i = 0; i < numNewVerts; i++)
            {
                // use vertex lookup to avoid creating duplicate vertices
                unsigned int vertIdx = mesh.GetVertIndex(polyVertices[vertOrdering[numNewVerts - 1 - i]], VertexPos, VertLookup);
                newFaceIdx.push_back(vertIdx);
            }
        }
        FaceIndices.push_back(newFaceIdx);
        NumberPolygons[numNewVerts] += 1;
    }

    mesh.DestroyTopology();
    mesh.DestroyRenderData();
    mesh.m_VertexPos = std::move(VertexPos);
    mesh.m_FaceIndices = std::move(FaceIndices);
    mesh.m_NumPolygons = std::move(NumberPolygons);
    mesh.TriangulateFaces();
}

////////// algorithms //////////

// Doo Sabin subdivision surface algorithm
void SubdivideDooSabin(Mesh& mesh)
{
    mesh.BuildTopology();

    // make new vertices and store original vertex connectivity
    unsigned int numFaces = static_cast<unsigned int>(mesh.m_TopoFaces.size());
    std::vector<std::vector<glm::vec3>> newPointsPerFace(numFaces);
    std::unordered_map<unsigned int, std::vector<glm::vec3>> pointsPerVertex;

    for (unsigned int currFaceIdx = 0; currFaceIdx < numFaces; currFaceIdx++)
    {
        FaceRecord currFace = mesh.m_TopoFaces[currFaceIdx];

        // associate new vertices with the original vertices and edges
        for (unsigned int vert : currFace.verticesIdx)
        {
            // point between vertex, face point, 2 neighbour edge points
            glm::vec3 point = 0.25f * (currFace.facePoint + mesh.m_TopoVertices[vert].position +
                mesh.m_TopoEdges[currFace.verticesEdges[vert][0]].midEdgePoint + mesh.m_TopoEdges[currFace.verticesEdges[vert][1]].midEdgePoint);
            // add to storage
            newPointsPerFace[currFaceIdx].push_back(point);
            pointsPerVertex[vert].push_back(point);
        }
    }

    std::unordered_map<unsigned int, std::vector<glm::vec3>> pointsPerEdge;
    for (unsigned int currEdgeIdx = 0; currEdgeIdx < mesh.m_TopoEdges.size(); currEdgeIdx++)
    {
        EdgeRecord currEdge = mesh.m_TopoEdges[currEdgeIdx];
        // skip boundary edges, they cannot form a new face
        if (currEdge.adjFacesIdx.size() == 2)
        {
            FaceRecord adjFace0 = mesh.m_TopoFaces[currEdge.adjFacesIdx[0]];
            FaceRecord adjFace1 = mesh.m_TopoFaces[currEdge.adjFacesIdx[1]];

            // add the new edge points in an ordered manner
            pointsPerEdge[currEdgeIdx].resize(4);
            pointsPerEdge[currEdgeIdx][0] =
                0.25f * (adjFace0.facePoint + mesh.m_TopoVertices[currEdge.endPoint1Idx].position +
                    mesh.m_TopoEdges[adjFace0.verticesEdges[currEdge.endPoint1Idx][0]].midEdgePoint + mesh.m_TopoEdges[adjFace0.verticesEdges[currEdge.endPoint1Idx][1]].midEdgePoint);
            pointsPerEdge[currEdgeIdx][1] =
                0.25f * (adjFace0.facePoint + mesh.m_TopoVertices[currEdge.endPoint2Idx].position +
                    mesh.m_TopoEdges[adjFace0.verticesEdges[currEdge.endPoint2Idx][0]].midEdgePoint + mesh.m_TopoEdges[adjFace0.verticesEdges[currEdge.endPoint2Idx][1]].midEdgePoint);
            pointsPerEdge[currEdgeIdx][2] =
                0.25f * (adjFace1.facePoint + mesh.m_TopoVertices[currEdge.endPoint2Idx].position +
                    mesh.m_TopoEdges[adjFace1.verticesEdges[currEdge.endPoint2Idx][0]].midEdgePoint + mesh.m_TopoEdges[adjFace1.verticesEdges[currEdge.endPoint2Idx][1]].midEdgePoint);
            pointsPerEdge[currEdgeIdx][3] =
                0.25f * (adjFace1.facePoint + mesh.m_TopoVertices[currEdge.endPoint1Idx].position +
                    mesh.m_TopoEdges[adjFace1.verticesEdges[currEdge.endPoint1Idx][0]].midEdgePoint + mesh.m_TopoEdges[adjFace1.verticesEdges[currEdge.endPoint1Idx][1]].midEdgePoint);
        }
    }

    DSWriteToMesh(mesh, newPointsPerFace, pointsPerVertex, pointsPerEdge);
}
