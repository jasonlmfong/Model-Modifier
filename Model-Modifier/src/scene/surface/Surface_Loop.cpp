# include "Surface.h"


////////// helpers to build the Object //////////

Object Surface::LoOutputOBJ(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (Loop style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<unsigned int, unsigned int> NumberPolygons;

    for (FaceRecord face : m_Faces)
    {
        unsigned int n = static_cast<unsigned int>(face.verticesIdx.size());

        std::vector<unsigned int> vertsIdx;
        std::vector<unsigned int> edgesIdx;
        for (unsigned int i = 0; i < n; i++)
        {
            glm::vec3 vert = m_Vertices[face.verticesIdx[i]].position;
            vertsIdx.push_back(getVertIndex(vert, VertexPos, VertLookup));

            glm::vec3 edge = edgePoints[getEdgeIndex({ face.verticesIdx[i], face.verticesIdx[(i + 1) % n] })];
            edgesIdx.push_back(getVertIndex(edge, VertexPos, VertLookup));
        }

        // every n-gon gets one n-gon inscribed inside, and gets n more triangles
        FaceIndices.push_back(edgesIdx);
        for (unsigned int i = 0; i < n; i++)
        {
            FaceIndices.push_back({ vertsIdx[i], edgesIdx[i], edgesIdx[(i + n - 1) % n] });
        }

        NumberPolygons[n] += 1;
        NumberPolygons[3] += n;
    }

    // build object
    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices;
    Obj.m_NumPolygons = NumberPolygons;
    Obj.TriangulateFaces();

    return Obj;
}


////////// algorithms //////////

// Loop subdivision surface algorithm
Object Surface::Loop()
{
    unsigned int numEdges = static_cast<unsigned int>(m_Edges.size());
    // make new (odd) vertices (per edge)
    std::vector<glm::vec3> edgePoints(numEdges);
    for (unsigned int i = 0; i < numEdges; i++)
    {
        EdgeRecord currEdge = m_Edges[i];
        if (currEdge.adjFacesIdx.size() == 1) // boundary edge
        {
            // ME point
            edgePoints[i] = currEdge.midEdgePoint;
        }
        else // edge borders 2 faces
        {
            // 3/8 face points + 2/8 edge point
            edgePoints[i] = 0.375f * m_Faces[currEdge.adjFacesIdx[0]].facePoint +
                0.375f * m_Faces[currEdge.adjFacesIdx[1]].facePoint +
                0.25f * currEdge.midEdgePoint;
        }
    }

    // update old (even) vertices (per vertex)
    unsigned int numVertices = static_cast<unsigned int>(m_Vertices.size());
    for (unsigned int i = 0; i < numVertices; i++)
    {
        VertexRecord vert = m_Vertices[i];
        glm::vec3 vertPos = vert.position;
        float alpha = 0.625f;
        glm::vec3 sumNeighbours{ 0 };
        for (unsigned int adjEdge : vert.adjEdgesIdx)
        {
            sumNeighbours += 2.0f * m_Edges[adjEdge].midEdgePoint - vertPos;
        }
        unsigned int neighbours = static_cast<unsigned int>(vert.adjEdgesIdx.size());
        if (neighbours == 2)
            m_Vertices[i].position = 0.75f * vertPos + 0.125f * sumNeighbours;
        else
        {
            float invNeigh = 1 / (float)neighbours;
            m_Vertices[i].position = (1 - alpha) * sumNeighbours * invNeigh + alpha * vertPos;
        }
    }

    return LoOutputOBJ(edgePoints);
}
