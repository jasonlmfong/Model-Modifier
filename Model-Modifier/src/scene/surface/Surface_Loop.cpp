# include "Surface.h"


////////// helpers to build the Object //////////

Object Surface::LoOutputOBJ3(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (Loop style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<int, int> NumberPolygons;

    for (FaceRecord face : m_Faces)
    {
        glm::vec3 vertA = m_Vertices[face.verticesIdx[0]].position;
        glm::vec3 vertB = m_Vertices[face.verticesIdx[1]].position;
        glm::vec3 vertC = m_Vertices[face.verticesIdx[2]].position;

        glm::vec3 edgeAB = edgePoints[getEdgeIndex({ face.verticesIdx[0], face.verticesIdx[1] })];
        glm::vec3 edgeBC = edgePoints[getEdgeIndex({ face.verticesIdx[1], face.verticesIdx[2] })];
        glm::vec3 edgeCA = edgePoints[getEdgeIndex({ face.verticesIdx[2], face.verticesIdx[0] })];

        // use vertex lookup to avoid creating duplicate vertices
        unsigned int vertAIdx = getVertIndex(vertA, VertexPos, VertLookup);
        unsigned int vertBIdx = getVertIndex(vertB, VertexPos, VertLookup);
        unsigned int vertCIdx = getVertIndex(vertC, VertexPos, VertLookup);
        unsigned int edgeABIdx = getVertIndex(edgeAB, VertexPos, VertLookup);
        unsigned int edgeBCIdx = getVertIndex(edgeBC, VertexPos, VertLookup);
        unsigned int edgeCAIdx = getVertIndex(edgeCA, VertexPos, VertLookup);

        // create new faces (each original traingle will have 4 new triangles
        FaceIndices.push_back({ vertAIdx, edgeABIdx, edgeCAIdx });
        FaceIndices.push_back({ edgeABIdx, vertBIdx, edgeBCIdx });
        FaceIndices.push_back({ edgeCAIdx, edgeBCIdx, vertCIdx });
        FaceIndices.push_back({ edgeCAIdx, edgeABIdx, edgeBCIdx });
        NumberPolygons[3] += 4;
    }

    // build object
    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices; Obj.m_TriFaceIndices = FaceIndices;
    Obj.m_NumPolygons = NumberPolygons;

    return Obj;
}

Object Surface::LoOutputOBJ4(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (Loop style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<int, int> NumberPolygons;

    for (FaceRecord face : m_Faces)
    {
        glm::vec3 vertA = m_Vertices[face.verticesIdx[0]].position;
        glm::vec3 vertB = m_Vertices[face.verticesIdx[1]].position;
        glm::vec3 vertC = m_Vertices[face.verticesIdx[2]].position;
        glm::vec3 vertD = m_Vertices[face.verticesIdx[3]].position;

        glm::vec3 edgeAB = edgePoints[getEdgeIndex({ face.verticesIdx[0], face.verticesIdx[1] })];
        glm::vec3 edgeBC = edgePoints[getEdgeIndex({ face.verticesIdx[1], face.verticesIdx[2] })];
        glm::vec3 edgeCD = edgePoints[getEdgeIndex({ face.verticesIdx[2], face.verticesIdx[3] })];
        glm::vec3 edgeDA = edgePoints[getEdgeIndex({ face.verticesIdx[3], face.verticesIdx[0] })];

        // use vertex lookup to avoid creating duplicate vertices
        unsigned int vertAIdx = getVertIndex(vertA, VertexPos, VertLookup);
        unsigned int vertBIdx = getVertIndex(vertB, VertexPos, VertLookup);
        unsigned int vertCIdx = getVertIndex(vertC, VertexPos, VertLookup);
        unsigned int vertDIdx = getVertIndex(vertD, VertexPos, VertLookup);
        unsigned int edgeABIdx = getVertIndex(edgeAB, VertexPos, VertLookup);
        unsigned int edgeBCIdx = getVertIndex(edgeBC, VertexPos, VertLookup);
        unsigned int edgeCDIdx = getVertIndex(edgeCD, VertexPos, VertLookup);
        unsigned int edgeDAIdx = getVertIndex(edgeDA, VertexPos, VertLookup);
        unsigned int FaceIdx = getVertIndex(face.facePoint, VertexPos, VertLookup);

        // create new faces (each original traingle will have 4 new triangles
        FaceIndices.push_back({ vertAIdx, edgeABIdx, FaceIdx, edgeDAIdx });
        FaceIndices.push_back({ vertBIdx, edgeBCIdx, FaceIdx, edgeABIdx });
        FaceIndices.push_back({ vertCIdx, edgeCDIdx, FaceIdx, edgeBCIdx });
        FaceIndices.push_back({ vertDIdx, edgeDAIdx, FaceIdx, edgeCDIdx });
        NumberPolygons[4] += 4;
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
Object Surface::Loop3()
{
    // make new (odd) vertices (per edge)
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
            // 3/8 face points + 2/8 edge point
            edgePoints[i] = 0.375f * m_Faces[currEdge.adjFacesIdx[0]].facePoint +
                0.375f * m_Faces[currEdge.adjFacesIdx[1]].facePoint +
                0.25f * currEdge.midEdgePoint;
        }
    }

    // update old (even) vertices (per vertex)
    for (int i = 0; i < m_Vertices.size(); i++)
    {
        VertexRecord vert = m_Vertices[i];
        glm::vec3 vertPos = vert.position;
        float alpha = 0.625f;
        glm::vec3 sumNeighbours{ 0 };
        for (unsigned int adjEdge : vert.adjEdgesIdx)
        {
            sumNeighbours += 2.0f * m_Edges[adjEdge].midEdgePoint - vertPos;
        }
        int neighbours = static_cast<int>(vert.adjEdgesIdx.size());
        if (neighbours == 2)
            m_Vertices[i].position = 0.75f * vertPos + 0.125f * sumNeighbours;
        else
        {
            float invNeigh = 1 / (float)neighbours;
            m_Vertices[i].position = (1 - alpha) * sumNeighbours * invNeigh + alpha * vertPos;
        }
    }

    return LoOutputOBJ3(edgePoints);
}

// Loop subdivision surface algorithm
Object Surface::Loop4()
{
    // make new (odd) vertices (per edge)
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
            // 3/8 face points + 2/8 edge point
            edgePoints[i] = 0.375f * m_Faces[currEdge.adjFacesIdx[0]].facePoint +
                0.375f * m_Faces[currEdge.adjFacesIdx[1]].facePoint +
                0.25f * currEdge.midEdgePoint;
        }
    }

    // update old (even) vertices (per vertex)
    for (int i = 0; i < m_Vertices.size(); i++)
    {
        VertexRecord vert = m_Vertices[i];
        glm::vec3 vertPos = vert.position;
        float alpha = 0.2f;
        glm::vec3 sumNeighbours{ 0 };
        for (unsigned int adjEdge : vert.adjEdgesIdx)
        {
            sumNeighbours += 2.0f * m_Edges[adjEdge].midEdgePoint - vertPos;
        }
        int neighbours = static_cast<int>(vert.adjEdgesIdx.size());
        if (neighbours == 3)
            m_Vertices[i].position = 0.25f * vertPos + 0.125f * sumNeighbours;
        else
        {
            float invNeigh = 1 / (float)neighbours;
            m_Vertices[i].position = (1 - alpha) * sumNeighbours * invNeigh + alpha * vertPos;
        }
    }

    return LoOutputOBJ4(edgePoints);
}
