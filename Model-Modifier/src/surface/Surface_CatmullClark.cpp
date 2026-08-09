#include "Surface.h"

////////// helpers to build the Mesh //////////

static std::vector<glm::vec3> CatmullClarkEdgePoints(Mesh& mesh)
{
    // calcuate edge points
    unsigned int numEdges = static_cast<unsigned int>(mesh.m_TopoEdges.size());
    std::vector<glm::vec3> edgePoints(numEdges);
    for (unsigned int i = 0; i < numEdges; i++)
    {
        EdgeRecord currEdge = mesh.m_TopoEdges[i];
        if (currEdge.adjFacesIdx.size() == 1) // boundary edge
        {
            // ME point
            edgePoints[i] = currEdge.midEdgePoint;
        }
        else // edge borders 2 faces
        {
            // (AF + ME) / 2 point
            edgePoints[i] = 0.5f * currEdge.midEdgePoint + 0.25f * (mesh.m_TopoFaces[currEdge.adjFacesIdx[0]].facePoint + mesh.m_TopoFaces[currEdge.adjFacesIdx[1]].facePoint);
        }
    }

    return edgePoints;
}

// create the mesh, in Catmull Clark style
static void CCWriteToMesh(Mesh& mesh, std::vector<glm::vec3>& edgePoints)
{
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<unsigned int, unsigned int> NumberPolygons;

    for (FaceRecord& face : mesh.m_TopoFaces)
    {
        unsigned int n = static_cast<unsigned int>(face.verticesIdx.size());

        std::vector<unsigned int> vertsIdx;
        std::vector<unsigned int> edgesIdx;
        for (unsigned int i = 0; i < n; i++)
        {
            glm::vec3 vert = mesh.m_TopoVertices[face.verticesIdx[i]].position;
            vertsIdx.push_back(mesh.GetVertIndex(vert, VertexPos, VertLookup));

            glm::vec3 edge = edgePoints[mesh.GetEdgeIndex({ face.verticesIdx[i], face.verticesIdx[(i + 1) % n] })];
            edgesIdx.push_back(mesh.GetVertIndex(edge, VertexPos, VertLookup));
        }

        glm::vec3 facePoint = face.facePoint;
        unsigned int facePointIdx = mesh.GetVertIndex(facePoint, VertexPos, VertLookup);

        // every n-gon turns into n quads
        for (unsigned int i = 0; i < n; i++)
        {
            FaceIndices.push_back({ vertsIdx[i], edgesIdx[i], facePointIdx, edgesIdx[(i + n - 1) % n] });
        }

        NumberPolygons[4] += n;
    }

    mesh.DestroyTopology();
    mesh.DestroyRenderData();
    mesh.m_VertexPos = std::move(VertexPos);
    mesh.m_FaceIndices = std::move(FaceIndices);
    mesh.m_NumPolygons = std::move(NumberPolygons);
    mesh.TriangulateFaces();
}

////////// algorithms //////////

// My own algorithm
void SubdivideBeehive(Mesh& mesh)
{
    mesh.BuildTopology();
    std::vector<glm::vec3> edgePoints = CatmullClarkEdgePoints(mesh);

    // update original vertex positions
    for (unsigned int i = 0; i < mesh.m_TopoVertices.size(); i++)
    {
        VertexRecord currVert = mesh.m_TopoVertices[i];

        // calculate F: average of face points 
        glm::vec3 avgFacePosition{ 0 };
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += mesh.m_TopoFaces[faceIdx].facePoint;
        }
        avgFacePosition /= 3 * static_cast<float>(currVert.adjFacesIdx.size());

        // update original vertex point to new position
        mesh.m_TopoVertices[i].position = avgFacePosition;
    }

    // build new mesh
    CCWriteToMesh(mesh, edgePoints);
}

// My own algorithm
void SubdivideSnowflake(Mesh& mesh)
{
    mesh.BuildTopology();
    std::vector<glm::vec3> edgePoints = CatmullClarkEdgePoints(mesh);

    // update original vertex positions
    for (unsigned int i = 0; i < mesh.m_TopoVertices.size(); i++)
    {
        VertexRecord currVert = mesh.m_TopoVertices[i];

        // calculate F: average of face points 
        glm::vec3 avgFacePosition{ 0 };
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += mesh.m_TopoFaces[faceIdx].facePoint;
        }
        avgFacePosition /= static_cast<float>(currVert.adjFacesIdx.size());

        // update original vertex point to new position
        mesh.m_TopoVertices[i].position = avgFacePosition;
    }

    // build new mesh
    CCWriteToMesh(mesh, edgePoints);
}

// Catmull Clark subdivision surface algorithm
void SubdivideCatmullClark(Mesh& mesh)
{
    mesh.BuildTopology();
    std::vector<glm::vec3> edgePoints = CatmullClarkEdgePoints(mesh);

    unsigned int numVertices = static_cast<unsigned int>(mesh.m_TopoVertices.size());
    // update original vertex positions
    std::vector<glm::vec3> originalPoints(numVertices);
    // store the original positions
    for (unsigned int i = 0; i < numVertices; i++)
    {
        originalPoints[i] = mesh.m_TopoVertices[i].position;
    }

    for (unsigned int i = 0; i < numVertices; i++)
    {
        VertexRecord currVert = mesh.m_TopoVertices[i];

        // calculate F: average of face points 
        glm::vec3 avgFacePosition{ 0 };
        for (unsigned int faceIdx : currVert.adjFacesIdx)
        {
            avgFacePosition += mesh.m_TopoFaces[faceIdx].facePoint;
        }
        float numAdjFaces = static_cast<float>(currVert.adjFacesIdx.size());
        avgFacePosition /= numAdjFaces;

        // calcalate R: average of edge midpoints
        glm::vec3 avgMidEdge{ 0 };
        for (unsigned int edgeIndex : currVert.adjEdgesIdx)
        {
            EdgeRecord currEdge = mesh.m_TopoEdges[edgeIndex];
            avgMidEdge += 0.5f * (originalPoints[currEdge.endPoint1Idx] + originalPoints[currEdge.endPoint2Idx]);
        }
        avgMidEdge /= static_cast<float>(currVert.adjEdgesIdx.size());

        glm::vec3 newPoint = avgFacePosition + 2.0f * avgMidEdge + (numAdjFaces - 3) * originalPoints[i];
        newPoint /= numAdjFaces;

        // update original vertex point to new position
        mesh.m_TopoVertices[i].position = newPoint;
    }

    // build new mesh
    CCWriteToMesh(mesh, edgePoints);
}
