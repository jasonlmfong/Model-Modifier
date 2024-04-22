#include "Surface.h"


////////// helpers to build the Object //////////

Object Surface::DSOutputOBJ(
    std::vector<std::vector<glm::vec3>> newPointsPerFace,
    std::unordered_map<unsigned int, std::vector<glm::vec3>> pointsPerVertex,
    std::unordered_map<unsigned int, std::vector<glm::vec3>> pointsPerEdge
)
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

    // build new Object class (DS style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<int, int> NumberPolygons;

    // new face from old face (n-gon from n-gon)
    for (int currFaceIdx = 0; currFaceIdx < m_Faces.size(); currFaceIdx++)
    {
        int n = static_cast<int>(newPointsPerFace[currFaceIdx].size());

        std::vector<unsigned int> newFaceIdx;
        for (int i = 0; i < n; i++)
        {
            // use vertex lookup to avoid creating duplicate vertices
            unsigned int vertIdx = getVertIndex(newPointsPerFace[currFaceIdx][i], VertexPos, VertLookup);
            newFaceIdx.push_back(vertIdx);
        }

        FaceIndices.push_back(newFaceIdx);
        NumberPolygons[n] += 1;
    }

    // new face from old edge (always a quad face)
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
                avgFaceNormal += ComputeFaceNormal(m_Faces[currEdge.adjFacesIdx[adjFaces]]);
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
    for (int currVertIdx = 0; currVertIdx < m_Vertices.size(); currVertIdx++)
    {
        VertexRecord currVert = m_Vertices[currVertIdx];
        if (currVert.adjFacesIdx.size() < 3)
            continue;

        // build neighbour face normal to match later
        glm::vec3 avgFaceNormal{ 0 };
        for (unsigned int adjFaceIdx : currVert.adjFacesIdx)
        {
            avgFaceNormal += ComputeFaceNormal(m_Faces[adjFaceIdx]);
        }
        avgFaceNormal /= static_cast<float>(currVert.adjFacesIdx.size());

        // get vertices for new face
        std::vector<glm::vec3> polyVertices;
        glm::vec3 centroid{ 0 };  // new facepoint
        int numNewVerts = static_cast<int>(pointsPerVertex[currVertIdx].size());
        for (int newVertIdx = 0; newVertIdx < numNewVerts; newVertIdx++)
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
        glm::vec3 vertFaceNormal = ComputeFaceNormal(polyVertices[vertOrdering[0]], polyVertices[vertOrdering[1]], polyVertices[vertOrdering[2]]);
        std::vector<unsigned int> newFaceIdx;
        if (glm::dot(vertFaceNormal, avgFaceNormal) > 0)
        {
            // not flipped, use the poylgon vertex ordering we have
            for (int i = 0; i < numNewVerts; i++)
            {
                // use vertex lookup to avoid creating duplicate vertices
                unsigned int vertIdx = getVertIndex(polyVertices[vertOrdering[i]], VertexPos, VertLookup);
                newFaceIdx.push_back(vertIdx);
            }
            FaceIndices.push_back(newFaceIdx);
        }
        else
        {
            // flipped normals, reverse the poylgon vertex ordering we have
            for (int i = 0; i < numNewVerts; i++)
            {
                // use vertex lookup to avoid creating duplicate vertices
                unsigned int vertIdx = getVertIndex(polyVertices[vertOrdering[numNewVerts - 1 - i]], VertexPos, VertLookup);
                newFaceIdx.push_back(vertIdx);
            }
            FaceIndices.push_back(newFaceIdx);
        }
        NumberPolygons[numNewVerts] += 1;
    }

    // build object
    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices;
    Obj.m_NumPolygons = NumberPolygons;
    Obj.TriangulateFaces();

    return Obj;
}