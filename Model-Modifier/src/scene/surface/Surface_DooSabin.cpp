#include "Surface.h"


////////// helpers to build the Object //////////

Object Surface::DSOutputOBJ(std::vector<glm::vec3> edgePoints)
{
    
}


////////// algorithms //////////

// Doo Sabin subdivision surface algorithm
Object Surface::DooSabin()
{
    // make new vertices and store original vertex connectivity
    std::vector<std::vector<glm::vec3>> newPointsPerFace(m_Faces.size());
    std::unordered_map<unsigned int, std::vector<glm::vec3>> pointsPerVertex;
    for (int currFaceIdx = 0; currFaceIdx < m_Faces.size(); currFaceIdx++)
    {
        FaceRecord currFace = m_Faces[currFaceIdx];

        // associate new vertices with the original vertices and edges
        for (unsigned int vert : currFace.verticesIdx)
        {
            // point between vertex, face point, 2 neighbour edge points
            glm::vec3 point = 0.25f * (currFace.facePoint + m_Vertices[vert].position +
                m_Edges[currFace.verticesEdges[vert][0]].midEdgePoint + m_Edges[currFace.verticesEdges[vert][1]].midEdgePoint);
            // add to storage
            newPointsPerFace[currFaceIdx].push_back(point);
            pointsPerVertex[vert].push_back(point);
        }
    }
    std::unordered_map<unsigned int, std::vector<glm::vec3>> pointsPerEdge;
    for (int currEdgeIdx = 0; currEdgeIdx < m_Edges.size(); currEdgeIdx++)
    {
        EdgeRecord currEdge = m_Edges[currEdgeIdx];
        // skip boundary edges, they cannot form a new face
        if (currEdge.adjFacesIdx.size() == 2)
        {
            FaceRecord adjFace0 = m_Faces[currEdge.adjFacesIdx[0]];
            FaceRecord adjFace1 = m_Faces[currEdge.adjFacesIdx[1]];

            // add the new edge points in an ordered manner
            pointsPerEdge[currEdgeIdx].resize(4);
            pointsPerEdge[currEdgeIdx][0] =
                0.25f * (adjFace0.facePoint + m_Vertices[currEdge.endPoint1Idx].position +
                    m_Edges[adjFace0.verticesEdges[currEdge.endPoint1Idx][0]].midEdgePoint + m_Edges[adjFace0.verticesEdges[currEdge.endPoint1Idx][1]].midEdgePoint);
            pointsPerEdge[currEdgeIdx][1] =
                0.25f * (adjFace0.facePoint + m_Vertices[currEdge.endPoint2Idx].position +
                    m_Edges[adjFace0.verticesEdges[currEdge.endPoint2Idx][0]].midEdgePoint + m_Edges[adjFace0.verticesEdges[currEdge.endPoint2Idx][1]].midEdgePoint);
            pointsPerEdge[currEdgeIdx][2] =
                0.25f * (adjFace1.facePoint + m_Vertices[currEdge.endPoint2Idx].position +
                    m_Edges[adjFace1.verticesEdges[currEdge.endPoint2Idx][0]].midEdgePoint + m_Edges[adjFace1.verticesEdges[currEdge.endPoint2Idx][1]].midEdgePoint);
            pointsPerEdge[currEdgeIdx][3] =
                0.25f * (adjFace1.facePoint + m_Vertices[currEdge.endPoint1Idx].position +
                    m_Edges[adjFace1.verticesEdges[currEdge.endPoint1Idx][0]].midEdgePoint + m_Edges[adjFace1.verticesEdges[currEdge.endPoint1Idx][1]].midEdgePoint);
        }
    }

    /*return DSOutputOBJ(edgePoints);*/

    // build new Object class (DS style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<int, int> NumberPolygons;

    // new face from old face
    for (int currFaceIdx = 0; currFaceIdx < m_Faces.size(); currFaceIdx++)
    {
        // use vertex lookup to avoid creating duplicate vertices
        unsigned int vertAIdx = getVertIndex(newPointsPerFace[currFaceIdx][0], VertexPos, VertLookup);
        unsigned int vertBIdx = getVertIndex(newPointsPerFace[currFaceIdx][1], VertexPos, VertLookup);
        unsigned int vertCIdx = getVertIndex(newPointsPerFace[currFaceIdx][2], VertexPos, VertLookup);

        FaceIndices.push_back({ vertAIdx, vertBIdx, vertCIdx });
        NumberPolygons[3] += 1;
    }

    // new face from old edge (broken down to 2 triangles)
    for (int currEdgeIdx = 0; currEdgeIdx < pointsPerEdge.size(); currEdgeIdx++)
    {
        EdgeRecord currEdge = m_Edges[currEdgeIdx];
        // skip boundary edges, they cannot form a new face
        if (currEdge.adjFacesIdx.size() == 2)
        {
            // build neighbour face normal to match later
            glm::vec3 avgFaceNormal{ 0 };
            for (int adjFaces = 0; adjFaces < 2; adjFaces++)
            {
                FaceRecord adjFace = m_Faces[currEdge.adjFacesIdx[adjFaces]];
                avgFaceNormal += ComputeFaceNormal(adjFace);
            }
            avgFaceNormal /= 2.0f;

            if (pointsPerEdge[currEdgeIdx].size() == 4)
            {
                // use vertex lookup to avoid creating duplicate vertices
                unsigned int vertAIdx = getVertIndex(pointsPerEdge[currEdgeIdx][0], VertexPos, VertLookup);
                unsigned int vertBIdx = getVertIndex(pointsPerEdge[currEdgeIdx][1], VertexPos, VertLookup);
                unsigned int vertCIdx = getVertIndex(pointsPerEdge[currEdgeIdx][2], VertexPos, VertLookup);
                unsigned int vertDIdx = getVertIndex(pointsPerEdge[currEdgeIdx][3], VertexPos, VertLookup);

                // check if our normal is flipped or not
                glm::vec3 ABCNormal = ComputeFaceNormal(pointsPerEdge[currEdgeIdx][0], pointsPerEdge[currEdgeIdx][1], pointsPerEdge[currEdgeIdx][2]);
                if (glm::dot(ABCNormal, avgFaceNormal) > 0)
                {
                    // not flipped, usual triangulation of 012, 230
                    FaceIndices.push_back({ vertAIdx, vertBIdx, vertCIdx });
                    FaceIndices.push_back({ vertCIdx, vertDIdx, vertAIdx });
                    NumberPolygons[3] += 2;
                }
                else
                {
                    // flipped normals, use 103, 321
                    FaceIndices.push_back({ vertBIdx, vertAIdx, vertDIdx });
                    FaceIndices.push_back({ vertDIdx, vertCIdx, vertBIdx });
                    NumberPolygons[3] += 2;
                }
            }
        }
    }

    // new face from old vertex (broken down to n-2 triangles)
    for (int currVertIdx = 0; currVertIdx < m_Vertices.size(); currVertIdx++)
    {
        VertexRecord currVert = m_Vertices[currVertIdx];
        // build neighbour face normal to match later
        glm::vec3 avgFaceNormal{ 0 };
        for (unsigned int adjFaceIdx : currVert.adjFacesIdx)
        {
            FaceRecord adjFace = m_Faces[adjFaceIdx];
            avgFaceNormal += ComputeFaceNormal(adjFace);
        }
        avgFaceNormal /= static_cast<float>(currVert.adjFacesIdx.size());

        std::vector<unsigned int> newVertsIdx;
        glm::vec3 center{ 0 };
        int numNewVerts = static_cast<int>(pointsPerVertex[currVertIdx].size());
        for (int newVertIdx = 0; newVertIdx < numNewVerts; newVertIdx++)
        {
            glm::vec3 currPoint = pointsPerVertex[currVertIdx][newVertIdx];
            // use vertex lookup to avoid creating duplicate vertices
            newVertsIdx.push_back(getVertIndex(currPoint, VertexPos, VertLookup));
            center += currPoint;
        }

        center /= numNewVerts;
        unsigned int centerIdx = getVertIndex(center, VertexPos, VertLookup);

        // TODO: FIX THE "HOLES" WITH A MORE EFFICIENT METHOD
        // CURRENTLY USING A DOUBLE FOR LOOP TO COVER ALL HOLES
        for (int newVertIdx = 0; newVertIdx < numNewVerts; newVertIdx++)
        {
            for (int newVertIdx2 = 0; newVertIdx2 < numNewVerts; newVertIdx2++)
            {
                if (newVertIdx != newVertIdx2)
                {
                    glm::vec3 testNormal = ComputeFaceNormal(center, pointsPerVertex[currVertIdx][newVertIdx], pointsPerVertex[currVertIdx][newVertIdx2]);
                    if (glm::dot(testNormal, avgFaceNormal) > 0)
                    {
                        // not flipped
                        FaceIndices.push_back({ centerIdx, newVertsIdx[newVertIdx], newVertsIdx[newVertIdx2] });
                        NumberPolygons[3] += 1;
                    }
                    else
                    {
                        // flipped normals
                        FaceIndices.push_back({ centerIdx, newVertsIdx[newVertIdx2], newVertsIdx[newVertIdx] });
                        NumberPolygons[3] += 1;
                    }
                }
            }
        }
    }

    // build object
    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices; Obj.m_TriFaceIndices = FaceIndices;
    Obj.m_NumPolygons = NumberPolygons;

    return Obj;
}