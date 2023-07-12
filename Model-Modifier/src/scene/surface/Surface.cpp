#include "Surface.h"

unsigned int Surface::getVertIndex(glm::vec3 vertPos, 
    std::vector<glm::vec3>& AllVertexPos, 
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>>& VertIdxLookup)
{
    // search if pos x in our lookup
    auto searchx = VertIdxLookup.find(vertPos.x);
    if (searchx != VertIdxLookup.end())
    {
        // search if pos y in our lookup
        auto searchy = searchx->second.find(vertPos.y);
        if (searchy != searchx->second.end())
        {
            // search if pos z in our lookup
            auto searchz = searchy->second.find(vertPos.z);
            if (searchz != searchy->second.end())
            {
                // found, return vert index
                return searchz->second;
            }
        }
    }
    // not found, create new edge and insert at the end of m_Edges
    unsigned int newVertIdx = AllVertexPos.size();
    AllVertexPos.push_back(vertPos);
    VertIdxLookup[vertPos.x][vertPos.y].insert(std::make_pair(vertPos.z, newVertIdx));
    return newVertIdx;
}

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
    newEdge.midEdgePoint = 0.5f * (
        m_Vertices[orderedVertPair.x].position +
        m_Vertices[orderedVertPair.y].position
        );
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

        // calculate the face points
        glm::vec3 vertexSum{0};
        for (int j = 0; j < 3; j++)
        {
            vertexSum += m_Vertices[faceVertices[j]].position;
        }
        newFace.facePoint = vertexSum / 3.0f;

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

Object Surface::CCOutputOBJ(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (CC style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;

    for (int faceIdx = 0; faceIdx < m_Faces.size(); faceIdx++)
    {
        FaceRecord face = m_Faces[faceIdx];

        glm::vec3 vertA = m_Vertices[face.verticesIdx[0]].position;
        glm::vec3 vertB = m_Vertices[face.verticesIdx[1]].position;
        glm::vec3 vertC = m_Vertices[face.verticesIdx[2]].position;

        glm::vec3 edgeAB = edgePoints[getEdgeIndex({ face.verticesIdx[0], face.verticesIdx[1] })];
        glm::vec3 edgeBC = edgePoints[getEdgeIndex({ face.verticesIdx[1], face.verticesIdx[2] })];
        glm::vec3 edgeCA = edgePoints[getEdgeIndex({ face.verticesIdx[2], face.verticesIdx[0] })];

        glm::vec3 faceABC = m_Faces[faceIdx].facePoint;

        // use vertex lookup to avoid creating duplicate vertices
        unsigned int vertAIdx = getVertIndex(vertA, VertexPos, VertLookup);
        unsigned int vertBIdx = getVertIndex(vertB, VertexPos, VertLookup);
        unsigned int vertCIdx = getVertIndex(vertC, VertexPos, VertLookup);
        unsigned int edgeABIdx = getVertIndex(edgeAB, VertexPos, VertLookup);
        unsigned int edgeBCIdx = getVertIndex(edgeBC, VertexPos, VertLookup);
        unsigned int edgeCAIdx = getVertIndex(edgeCA, VertexPos, VertLookup);
        unsigned int faceABCIdx = getVertIndex(faceABC, VertexPos, VertLookup);

        // my variation to return a triangle mesh: connect the original vertex with face point, so the quads will become 2 triangles
        FaceIndices.push_back({ vertAIdx, edgeABIdx, faceABCIdx });
        FaceIndices.push_back({ faceABCIdx, edgeCAIdx, vertAIdx });

        FaceIndices.push_back({ vertBIdx, edgeBCIdx, faceABCIdx });
        FaceIndices.push_back({ faceABCIdx, edgeABIdx, vertBIdx });

        FaceIndices.push_back({ vertCIdx, edgeCAIdx, faceABCIdx });
        FaceIndices.push_back({ faceABCIdx, edgeBCIdx, vertCIdx });
    }

    Object Object;
    Object.m_Min = m_Min; Object.m_Max = m_Max;
    Object.m_VertexPos = VertexPos; Object.m_FaceIndices = FaceIndices;

    return Object;
}

////////// algorithms //////////

// My own algorithm
Object Surface::Beehive()
{
    // calcuate edge points
    std::vector<glm::vec3> edgePoints(m_Edges.size());
    for (int i = 0; i < edgePoints.size(); i++)
    {
        EdgeRecord currEdge = m_Edges[i];
        if (currEdge.adjFacesIdx.size() == 1) // boundary edge
        {
            // ME point
            edgePoints[i] = currEdge.midEdgePoint;
        }
        else // edge borders 2 faces
        {
            // (AF + ME) / 2 point
            edgePoints[i] = 0.5f * currEdge.midEdgePoint + 0.25f * (m_Faces[currEdge.adjFacesIdx[0]].facePoint + m_Faces[currEdge.adjFacesIdx[1]].facePoint);
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
            avgFacePosition += m_Faces[faceIdx].facePoint;
        }
        float numAdjFaces = currVert.adjFacesIdx.size();
        avgFacePosition /= 3 * numAdjFaces;

        // update original vertex point to new position
        m_Vertices[i].position = avgFacePosition;
    }

    // build new Object class
    return CCOutputOBJ(edgePoints);
}

// My own algorithm
Object Surface::Snowflake()
{
    // calcuate edge points
    std::vector<glm::vec3> edgePoints(m_Edges.size());
    for (int i = 0; i < edgePoints.size(); i++)
    {
        EdgeRecord currEdge = m_Edges[i];
        if (currEdge.adjFacesIdx.size() == 1) // boundary edge
        {
            // ME point
            edgePoints[i] = currEdge.midEdgePoint;
        }
        else // edge borders 2 faces
        {
            // (AF + ME) / 2 point
            edgePoints[i] = 0.5f * currEdge.midEdgePoint + 0.25f * (m_Faces[currEdge.adjFacesIdx[0]].facePoint + m_Faces[currEdge.adjFacesIdx[1]].facePoint);
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
            avgFacePosition += m_Faces[faceIdx].facePoint;
        }
        float numAdjFaces = currVert.adjFacesIdx.size();
        avgFacePosition /= numAdjFaces;

        // update original vertex point to new position
        m_Vertices[i].position = avgFacePosition;
    }

    // build new Object class
    return CCOutputOBJ(edgePoints);
}

// Catmull Clark subdivision surface algorithm
Object Surface::CatmullClark()
{
    // calcuate edge points
    std::vector<glm::vec3> edgePoints(m_Edges.size());
    for (int i = 0; i < edgePoints.size(); i++)
    {
        EdgeRecord currEdge = m_Edges[i];
        if (currEdge.adjFacesIdx.size() == 1) // boundary edge
        {
            // ME point
            edgePoints[i] = currEdge.midEdgePoint;
        }
        else // edge borders 2 faces
        {
            // (AF + ME) / 2 point
            edgePoints[i] = 0.5f * currEdge.midEdgePoint + 0.25f * (m_Faces[currEdge.adjFacesIdx[0]].facePoint + m_Faces[currEdge.adjFacesIdx[1]].facePoint);
        }
    }

    // update original vertex positions
    std::vector<glm::vec3> originalPoints(m_Vertices.size());
    // store the original positions
    for (int i = 0; i < m_Vertices.size(); i++)
    {
        originalPoints[i] = m_Vertices[i].position;
    }
    for (int i = 0; i < m_Vertices.size(); i++)
    {
        VertexRecord currVert = m_Vertices[i];

        // calculate F: average of face points 
        glm::vec3 avgFacePosition{0};
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += m_Faces[faceIdx].facePoint;
        }
        float numAdjFaces = currVert.adjFacesIdx.size();
        avgFacePosition /= numAdjFaces;

        // calcalate R: average of edge midpoints
        glm::vec3 avgMidEdge{0};
        for (unsigned int edgeIndex : currVert.adjEdgesIdx)
        {
            EdgeRecord currEdge = m_Edges[edgeIndex];
            avgMidEdge += 0.5f * (originalPoints[currEdge.endPoint1Idx] + originalPoints[currEdge.endPoint2Idx]);
        }
        float numAdjEdges = currVert.adjEdgesIdx.size();
        avgMidEdge /= numAdjEdges;

        glm::vec3 newPoint = avgFacePosition + 2.0f * avgMidEdge + (numAdjFaces - 3) * originalPoints[i];
        newPoint /= numAdjFaces;

        // update original vertex point to new position
        m_Vertices[i].position = newPoint;
    }

    // build new Object class
    return CCOutputOBJ(edgePoints);
}
