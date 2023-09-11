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

////////// helper //////////

glm::vec3 Surface::ComputeFaceNormal(FaceRecord face)
{
    glm::vec3 pos0 = m_Vertices[face.verticesIdx[0]].position;
    glm::vec3 pos1 = m_Vertices[face.verticesIdx[1]].position;
    glm::vec3 pos2 = m_Vertices[face.verticesIdx[2]].position;
    return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
}

glm::vec3 Surface::ComputeFaceNormal(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2)
{
    return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
}

// create the obj file, in Catmull Clark style
Object Surface::CCOutputOBJ(std::vector<glm::vec3> edgePoints)
{
    // build new Object class (CC style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;

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
    }

    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices;

    return Obj;
}

// computer quadric matrix by summing all K_p matrices of a vertice v0
glm::mat4 Surface::ComputeQuadric(VertexRecord v0)
{
    glm::mat4 quadric{ 0.0f };
    // for each neighbouring face, compute K_p
    glm::vec3 position = v0.position;
    for (unsigned int faceIdx : v0.adjFacesIdx)
    {
        FaceRecord face = m_Faces[faceIdx];
        glm::vec3 faceNormal = ComputeFaceNormal(face);
        glm::vec4 plane{ faceNormal, -glm::dot(faceNormal, position) }; // plane equation ax+by+cz+d = 0

        quadric += glm::outerProduct(plane, plane); // K_p
    }

    return quadric;
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
        float numAdjFaces = static_cast<float>(currVert.adjFacesIdx.size());
        avgFacePosition /= numAdjFaces;

        // calcalate R: average of edge midpoints
        glm::vec3 avgMidEdge{0};
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

    // new face from old face
    for (int currFaceIdx = 0; currFaceIdx < m_Faces.size(); currFaceIdx++)
    {
        // use vertex lookup to avoid creating duplicate vertices
        unsigned int vertAIdx = getVertIndex(newPointsPerFace[currFaceIdx][0], VertexPos, VertLookup);
        unsigned int vertBIdx = getVertIndex(newPointsPerFace[currFaceIdx][1], VertexPos, VertLookup);
        unsigned int vertCIdx = getVertIndex(newPointsPerFace[currFaceIdx][2], VertexPos, VertLookup);

        FaceIndices.push_back({ vertAIdx, vertBIdx, vertCIdx });
    }

    // new face from old edge (broken down to 2 triangles)
    for (int currEdgeIdx = 0; currEdgeIdx < pointsPerEdge.size(); currEdgeIdx++)
    {
        EdgeRecord currEdge = m_Edges[currEdgeIdx];
        // skip boundary edges, they cannot form a new face
        if (currEdge.adjFacesIdx.size() == 2)
        {
            // build neighbour face normal to match later
            glm::vec3 avgFaceNormal{0};
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
                }
                else
                {
                    // flipped normals, use 103, 321
                    FaceIndices.push_back({ vertBIdx, vertAIdx, vertDIdx });
                    FaceIndices.push_back({ vertDIdx, vertCIdx, vertBIdx });
                }
            }
        }
    }

    // new face from old vertex (broken down to n-2 triangles)
    for (int currVertIdx = 0; currVertIdx < m_Vertices.size(); currVertIdx++)
    {
        VertexRecord currVert = m_Vertices[currVertIdx];
        // build neighbour face normal to match later
        glm::vec3 avgFaceNormal{0};
        for (unsigned int adjFaceIdx : currVert.adjFacesIdx)
        {
            FaceRecord adjFace = m_Faces[adjFaceIdx];
            avgFaceNormal += ComputeFaceNormal(adjFace);
        }
        avgFaceNormal /= static_cast<float>(currVert.adjFacesIdx.size());

        std::vector<unsigned int> newVertsIdx;
        glm::vec3 center{0};
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
                    }
                    else
                    {
                        // flipped normals
                        FaceIndices.push_back({ centerIdx, newVertsIdx[newVertIdx2], newVertsIdx[newVertIdx] });
                    }
                }
            }
        }
    }

    // build object
    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices;

    return Obj;
}

// Loop subdivision surface algorithm
Object Surface::Loop()
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
        glm::vec3 sumNeighbours {0};
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

    // build new Object class (Loop style)
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;

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
    }

    // build object
    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices;

    return Obj;
}
