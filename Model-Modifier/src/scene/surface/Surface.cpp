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
    unsigned int newVertIdx = static_cast<unsigned int>(AllVertexPos.size());
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
    unsigned int newEdgeIdx = static_cast<unsigned int>(m_Edges.size());
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
        unsigned int n = static_cast<unsigned int>(faceVertices.size());

        // create face record to be stored
        FaceRecord newFace;
        newFace.verticesIdx = faceVertices;

        // calculate the face points
        glm::vec3 vertexSum{0};
        for (unsigned int j = 0; j < n; j++)
        {
            vertexSum += m_Vertices[faceVertices[j]].position;
        }
        newFace.facePoint = vertexSum / ((float)n);

        // get edges next to the current face
        for (unsigned int i = 0; i < n; i++)
        {
            unsigned int startVertex = faceVertices[i];
            unsigned int endVertex = faceVertices[(i + 1) % n];
            unsigned int edgeIndex = getEdgeIndex({ startVertex, endVertex });

            newFace.verticesEdges[startVertex].push_back(edgeIndex);
            newFace.verticesEdges[endVertex].push_back(edgeIndex);
            newFace.edgesIdx.push_back(edgeIndex);

            // add the edges to the connecting vertices
            m_Vertices[startVertex].adjEdgesIdx.push_back(edgeIndex);
            m_Vertices[endVertex].adjEdgesIdx.push_back(edgeIndex);
        }

        unsigned int faceIndex = static_cast<unsigned int>(m_Faces.size());
        m_Faces.push_back(newFace);

        // add face index to the connecting vertices and edges 
        for (unsigned int i = 0; i < n; i++)
        {
            m_Vertices[faceVertices[i]].adjFacesIdx.push_back(faceIndex);
            m_Edges[newFace.edgesIdx[i]].adjFacesIdx.push_back(faceIndex);
        }
    }
}

Surface::~Surface()
{
}

////////// helper //////////

glm::vec3 Surface::ComputeFaceNormal(FaceRecord face)
{
    glm::vec3 pos0 = m_Vertices[face.verticesIdx[0]].position;
    glm::vec3 pos1 = m_Vertices[face.verticesIdx[1]].position;
    glm::vec3 pos2 = m_Vertices[face.verticesIdx[2]].position;
    return ComputeFaceNormal(pos0, pos1, pos2);
}

glm::vec3 Surface::ComputeFaceNormal(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2)
{
    return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
}
