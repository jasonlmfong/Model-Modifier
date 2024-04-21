#include "Surface.h"


////////// helpers to build the Object //////////

// create the obj file, in Catmull Clark style (for triangular meshes)
Object Surface::CCOutputOBJ3(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (CC style)
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

        glm::vec3 faceABC = face.facePoint;

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

        NumberPolygons[3] += 6;
    }

    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices; Obj.m_TriFaceIndices = FaceIndices;
    Obj.m_NumPolygons = NumberPolygons;

    return Obj;
}

// create the obj file, in Catmull Clark style (for quad meshes)
Object Surface::CCOutputOBJ4(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (CC style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<int, int> NumberPolygons;

    for (FaceRecord face : m_Faces)
    {
        //for (int vertIdx = 0; vertIdx < face.verticesIdx.size(); vertIdx++)
        //{

        //}
        glm::vec3 vertA = m_Vertices[face.verticesIdx[0]].position;
        glm::vec3 vertB = m_Vertices[face.verticesIdx[1]].position;
        glm::vec3 vertC = m_Vertices[face.verticesIdx[2]].position;
        glm::vec3 vertD = m_Vertices[face.verticesIdx[3]].position;

        glm::vec3 edgeAB = edgePoints[getEdgeIndex({ face.verticesIdx[0], face.verticesIdx[1] })];
        glm::vec3 edgeBC = edgePoints[getEdgeIndex({ face.verticesIdx[1], face.verticesIdx[2] })];
        glm::vec3 edgeCD = edgePoints[getEdgeIndex({ face.verticesIdx[2], face.verticesIdx[3] })];
        glm::vec3 edgeDA = edgePoints[getEdgeIndex({ face.verticesIdx[3], face.verticesIdx[0] })];

        glm::vec3 faceABCD = face.facePoint;

        // use vertex lookup to avoid creating duplicate vertices
        unsigned int vertAIdx = getVertIndex(vertA, VertexPos, VertLookup);
        unsigned int vertBIdx = getVertIndex(vertB, VertexPos, VertLookup);
        unsigned int vertCIdx = getVertIndex(vertC, VertexPos, VertLookup);
        unsigned int vertDIdx = getVertIndex(vertD, VertexPos, VertLookup);
        unsigned int edgeABIdx = getVertIndex(edgeAB, VertexPos, VertLookup);
        unsigned int edgeBCIdx = getVertIndex(edgeBC, VertexPos, VertLookup);
        unsigned int edgeCDIdx = getVertIndex(edgeCD, VertexPos, VertLookup);
        unsigned int edgeDAIdx = getVertIndex(edgeDA, VertexPos, VertLookup);
        unsigned int faceABCDIdx = getVertIndex(faceABCD, VertexPos, VertLookup);

        // my variation to return a triangle mesh: connect the original vertex with face point, so the quads will become 2 triangles
        FaceIndices.push_back({ vertAIdx, edgeABIdx, faceABCDIdx, edgeDAIdx });

        FaceIndices.push_back({ vertBIdx, edgeBCIdx, faceABCDIdx, edgeABIdx });

        FaceIndices.push_back({ vertCIdx, edgeCDIdx, faceABCDIdx, edgeBCIdx });

        FaceIndices.push_back({ vertDIdx, edgeDAIdx, faceABCDIdx, edgeCDIdx });

        NumberPolygons[4] += 4;
    }

    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices;
    Obj.m_NumPolygons = NumberPolygons;
    Obj.TriangulateFaces();
    return Obj;
}


////////// algorithms //////////

// My own algorithm
Object Surface::Beehive(int n)
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
        glm::vec3 avgFacePosition{ 0 };
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += m_Faces[faceIdx].facePoint;
        }
        avgFacePosition /= 3 * static_cast<float>(currVert.adjFacesIdx.size());

        // update original vertex point to new position
        m_Vertices[i].position = avgFacePosition;
    }

    // build new Object class
    if (n == 3)
        return CCOutputOBJ3(edgePoints);
    if (n == 4)
        return CCOutputOBJ4(edgePoints);
}

// My own algorithm
Object Surface::Snowflake(int n)
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
        glm::vec3 avgFacePosition{ 0 };
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += m_Faces[faceIdx].facePoint;
        }
        avgFacePosition /= static_cast<float>(currVert.adjFacesIdx.size());

        // update original vertex point to new position
        m_Vertices[i].position = avgFacePosition;
    }

    // build new Object class
    if (n == 3)
        return CCOutputOBJ3(edgePoints);
    if (n == 4)
        return CCOutputOBJ4(edgePoints);
}

// Catmull Clark subdivision surface algorithm
Object Surface::CatmullClark(int n)
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
        glm::vec3 avgFacePosition{ 0 };
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += m_Faces[faceIdx].facePoint;
        }
        float numAdjFaces = static_cast<float>(currVert.adjFacesIdx.size());
        avgFacePosition /= numAdjFaces;

        // calcalate R: average of edge midpoints
        glm::vec3 avgMidEdge{ 0 };
        for (unsigned int edgeIndex : currVert.adjEdgesIdx)
        {
            EdgeRecord currEdge = m_Edges[edgeIndex];
            avgMidEdge += 0.5f * (originalPoints[currEdge.endPoint1Idx] + originalPoints[currEdge.endPoint2Idx]);
        }
        avgMidEdge /= static_cast<float>(currVert.adjEdgesIdx.size());

        glm::vec3 newPoint = avgFacePosition + 2.0f * avgMidEdge + (numAdjFaces - 3) * originalPoints[i];
        newPoint /= numAdjFaces;

        // update original vertex point to new position
        m_Vertices[i].position = newPoint;
    }

    // build new Object class
    if (n == 3)
        return CCOutputOBJ3(edgePoints);
    if (n == 4)
        return CCOutputOBJ4(edgePoints);
}
