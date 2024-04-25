#include "Surface.h"


////////// helpers to build the Object //////////

std::vector<glm::vec3> Surface::CatmulClarkEdgePoints()
{
    // calcuate edge points
    unsigned int numEdges = m_Edges.size();
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
            // (AF + ME) / 2 point
            edgePoints[i] = 0.5f * currEdge.midEdgePoint + 0.25f * (m_Faces[currEdge.adjFacesIdx[0]].facePoint + m_Faces[currEdge.adjFacesIdx[1]].facePoint);
        }
    }

    return edgePoints;
}

// create the obj file, in Catmull Clark style
Object Surface::CCOutputOBJ(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (CC style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<int, int> NumberPolygons;

    for (FaceRecord face : m_Faces)
    {
        int n = static_cast<int>(face.verticesIdx.size());
        
        std::vector<unsigned int> vertsIdx;
        std::vector<unsigned int> edgesIdx;
        for (int i = 0; i < n; i++)
        {
            glm::vec3 vert = m_Vertices[face.verticesIdx[i]].position;
            vertsIdx.push_back(getVertIndex(vert, VertexPos, VertLookup));

            glm::vec3 edge = edgePoints[getEdgeIndex({ face.verticesIdx[i], face.verticesIdx[(i + 1) % n] })];
            edgesIdx.push_back(getVertIndex(edge, VertexPos, VertLookup));
        }

        glm::vec3 facePoint = face.facePoint;
        unsigned int facePointIdx = getVertIndex(facePoint, VertexPos, VertLookup);
            
        // every n-gon turns into n quads
        for (int i = 0; i < n; i++)
        {
            FaceIndices.push_back({ vertsIdx[i], edgesIdx[i], facePointIdx, edgesIdx[(i + n - 1) % n] });
        }

        NumberPolygons[4] += n;
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
Object Surface::Beehive()
{
    std::vector<glm::vec3> edgePoints = CatmulClarkEdgePoints();

    // update original vertex positions
    for (unsigned int i = 0; i < m_Vertices.size(); i++)
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
    return CCOutputOBJ(edgePoints);
}

// My own algorithm
Object Surface::Snowflake()
{
    std::vector<glm::vec3> edgePoints = CatmulClarkEdgePoints();

    // update original vertex positions
    for (unsigned int i = 0; i < m_Vertices.size(); i++)
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
    return CCOutputOBJ(edgePoints);
}

// Catmull Clark subdivision surface algorithm
Object Surface::CatmullClark()
{
    std::vector<glm::vec3> edgePoints = CatmulClarkEdgePoints();

    unsigned int numVertices = m_Vertices.size();
    // update original vertex positions
    std::vector<glm::vec3> originalPoints(numVertices);
    // store the original positions
    for (unsigned int i = 0; i < numVertices; i++)
    {
        originalPoints[i] = m_Vertices[i].position;
    }
    for (unsigned int i = 0; i < numVertices; i++)
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
    return CCOutputOBJ(edgePoints);
}
