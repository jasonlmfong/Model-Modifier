#include "Surface.h"

unsigned int Surface::getEdgeIndex(glm::uvec2 vertPair)
{
    glm::uvec2 orderedVertPair = vertPair;
    if (vertPair.x > vertPair.y) // check if the order is maintained (idx1 < idx2)
    {
        // if it is violated, swap it
        orderedVertPair.x = vertPair.y;
        orderedVertPair.y = vertPair.x;
    }

    // search if starting vertex in our lookup
    auto searchx = m_EdgeIdxLookup.find(orderedVertPair.x);
    if (searchx != m_EdgeIdxLookup.end())
    {
        // search if ending vertex in our lookup
        auto searchy = searchx->second.find(orderedVertPair.y);
        if (searchy != searchx->second.end())
        {
            // found, return edge index
            return searchy->second;
        }
    }
    // not found, create new edge and insert at the end of m_Edges
    unsigned int newEdgeIdx = m_Edges.size();
    EdgeRecord newEdge;
    newEdge.endPoint1Idx = orderedVertPair.x;
    newEdge.endPoint2Idx = orderedVertPair.y;
    m_Edges.push_back(newEdge);
    m_EdgeIdxLookup[orderedVertPair.x].insert(std::make_pair(orderedVertPair.y, newEdgeIdx));
    return newEdgeIdx;
}

Surface::Surface(Object obj)
    : m_Min(obj.m_Min), m_Max(obj.m_Max)
{
    for (glm::vec3 vertPos : obj.m_VertexPos)
    {
        // populate m_Vertices with obj.m_VertexPos data
        VertexRecord newVertex;
        newVertex.position = vertPos;
        m_Vertices.push_back(newVertex);
    }

    for (std::vector<unsigned int> faceVertices : obj.m_FaceIndices)
    {
        // create face record to be stored
        FaceRecord newFace;
        newFace.verticesIdx = faceVertices;

        // get edges next to the current face
        for (int i = 0; i < 3; i++)
        {
            unsigned int startVertex = faceVertices[i];
            unsigned int endVertex = faceVertices[(i + 1) % 3];
            unsigned int edgeIndex = getEdgeIndex({ startVertex, endVertex });

            newFace.edgesIdx.push_back(edgeIndex);

            // add the edges to the connecting vertices
            m_Vertices[startVertex].adjEdgesIdx.push_back(edgeIndex);
            m_Vertices[endVertex].adjEdgesIdx.push_back(edgeIndex);
        }

        unsigned int faceIndex = m_Faces.size();
        m_Faces.push_back(newFace);

        // add face index to the connecting vertices and edges 
        for (int i = 0; i < 3; i++)
        {
            m_Vertices[faceVertices[i]].adjFacesIdx.push_back(faceIndex);
            m_Edges[newFace.edgesIdx[i]].adjFacesIdx.push_back(faceIndex);
        }
    }
}

Surface::~Surface()
{
}

// My own algorithm
Object Surface::Beehive()
{
    // calculate the face points
    std::vector<glm::vec3> facePoints(m_Faces.size());
    for (int i = 0; i < facePoints.size(); i++)
    {
        FaceRecord currFace = m_Faces[i];

        glm::vec3 vertexSum{0};
        for (int j = 0; j < 3; j++)
        {
            vertexSum += m_Vertices[currFace.verticesIdx[j]].position;
        }
        facePoints[i] = vertexSum / 3.0f;
    }

    // calcuate edge points
    std::vector<glm::vec3> edgePoints(m_Edges.size());
    for (int i = 0; i < edgePoints.size(); i++)
    {
        EdgeRecord currEdge = m_Edges[i];
        if (currEdge.adjFacesIdx.size() == 1) // boundary edge
        {
            // ME point
            edgePoints[i] = 0.5f * (
                m_Vertices[currEdge.endPoint1Idx].position +
                m_Vertices[currEdge.endPoint2Idx].position
                );
        }
        else // edge borders 2 faces
        {
            // (AF + ME) / 2 point
            edgePoints[i] = 0.25f * (
                m_Vertices[currEdge.endPoint1Idx].position +
                m_Vertices[currEdge.endPoint2Idx].position +
                facePoints[currEdge.adjFacesIdx[0]] + facePoints[currEdge.adjFacesIdx[1]]);
        }
    }

    // update original vertex positions
    for (int i = 0; i < m_Vertices.size(); i++)
    {
        VertexRecord currVert = m_Vertices[i];

        // calculate F: average of face points 
        glm::vec3 avgFacePosition{0};
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += facePoints[faceIdx];
        }
        float numAdjFaces = currVert.adjFacesIdx.size();
        avgFacePosition /= 3 * numAdjFaces;

        // update original vertex point to new position
        m_Vertices[i].position = avgFacePosition;
    }

    // build new Object class
    std::vector<glm::vec3> BHVertexPos;
    std::vector<std::vector<unsigned int>> BHFaceIndices;

    for (int faceIdx = 0; faceIdx < m_Faces.size(); faceIdx++)
    {
        FaceRecord face = m_Faces[faceIdx];

        glm::vec3 vertA = m_Vertices[face.verticesIdx[0]].position;
        glm::vec3 vertB = m_Vertices[face.verticesIdx[1]].position;
        glm::vec3 vertC = m_Vertices[face.verticesIdx[2]].position;

        glm::vec3 edgeAB = edgePoints[getEdgeIndex({ face.verticesIdx[0], face.verticesIdx[1] })];
        glm::vec3 edgeBC = edgePoints[getEdgeIndex({ face.verticesIdx[1], face.verticesIdx[2] })];
        glm::vec3 edgeCA = edgePoints[getEdgeIndex({ face.verticesIdx[2], face.verticesIdx[0] })];

        glm::vec3 faceABC = facePoints[faceIdx];

        unsigned int vertAIdx = BHVertexPos.size();
        unsigned int vertBIdx = vertAIdx + 1;
        unsigned int vertCIdx = vertAIdx + 2;

        unsigned int edgeABIdx = vertAIdx + 3;
        unsigned int edgeBCIdx = vertAIdx + 4;
        unsigned int edgeCAIdx = vertAIdx + 5;

        unsigned int faceABCIdx = vertAIdx + 6;

        BHVertexPos.push_back({ vertA });
        BHVertexPos.push_back({ vertB });
        BHVertexPos.push_back({ vertC });
        BHVertexPos.push_back({ edgeAB });
        BHVertexPos.push_back({ edgeBC });
        BHVertexPos.push_back({ edgeCA });
        BHVertexPos.push_back({ faceABC });

        // my variation to return a triangle mesh: connect the original vertex with face point, so the quads will become 2 triangles
        BHFaceIndices.push_back({ vertAIdx, edgeABIdx, faceABCIdx });
        BHFaceIndices.push_back({ faceABCIdx, edgeCAIdx, vertAIdx });

        BHFaceIndices.push_back({ vertBIdx, edgeBCIdx, faceABCIdx });
        BHFaceIndices.push_back({ faceABCIdx, edgeABIdx, vertBIdx });

        BHFaceIndices.push_back({ vertCIdx, edgeCAIdx, faceABCIdx });
        BHFaceIndices.push_back({ faceABCIdx, edgeBCIdx, vertCIdx });
    }

    Object BHObject;
    BHObject.m_Min = m_Min; BHObject.m_Max = m_Max;
    BHObject.m_VertexPos = BHVertexPos; BHObject.m_FaceIndices = BHFaceIndices;

    return BHObject;
}
