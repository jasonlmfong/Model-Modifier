#include "Surface.h"


////////// helper //////////

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

Object Surface::GHOutputOBJ()
{
    // build new Object class
    std::vector<glm::vec3> VertexPos;
    std::unordered_map<float, std::unordered_map<float, std::unordered_map<float, unsigned int>>> VertLookup;
    std::vector<std::vector<unsigned int>> FaceIndices;
    std::unordered_map<unsigned int, unsigned int> NumberPolygons;

    for (VertexRecord vert : m_Vertices)
    {
        glm::vec3 vertPos = vert.position;
        getVertIndex(vertPos, VertexPos, VertLookup);
    }

    for (FaceRecord face : m_Faces)
    {
        std::vector<unsigned int> vertsIdx;
        for (unsigned int i = 0; i < 3; i++)
        {
            glm::vec3 vert = m_Vertices[face.verticesIdx[i]].position;
            vertsIdx.push_back(getVertIndex(vert, VertexPos, VertLookup));
        }
        FaceIndices.push_back(vertsIdx);
        NumberPolygons[3] += 1;
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

// Garland Heckbert simplification surface algorithm
Object Surface::QEM(unsigned int desiredCount)
{
    unsigned int numVertices = static_cast<unsigned int>(m_Vertices.size());
    // calculate quadric error for each vertex
    std::unordered_map<unsigned int, glm::mat4> quadricLookup;
    for (unsigned int i = 0; i < numVertices; i++)
    {
        quadricLookup.insert({ i, ComputeQuadric(m_Vertices[i]) });
    }

    const float THRESHOLD = 0.05f;

    // select all valid pairs
    std::vector<std::set<unsigned int>> vertexPairLookup; // maintain vertex pairs
    vertexPairLookup.resize(numVertices);

    std::vector<ValidPair> validPairs;
    for (unsigned int firstV = 0; firstV < numVertices; firstV++)
    {
        for (unsigned int secondV = firstV + 1; secondV < numVertices; secondV++)
        {
            auto searchx = m_EdgeIdxLookup.find(firstV);
            if (searchx != m_EdgeIdxLookup.end())
            {
                // search if ending vertex in our lookup
                auto searchy = searchx->second.find(secondV);
                if (searchy != searchx->second.end())
                {
                    // add the pair idx to the vertices
                    vertexPairLookup[firstV].insert(static_cast<unsigned int>(validPairs.size()));
                    vertexPairLookup[secondV].insert(static_cast<unsigned int>(validPairs.size()));
                    // found edge, add to valid pairs
                    ValidPair newPair{}; newPair.vertOne = firstV; newPair.vertTwo = secondV; newPair.edge = true;
                    validPairs.push_back(newPair);
                }
            }
            // not found, check if the edges are close (distance smaller than threshold)
            glm::vec3 firstPos = m_Vertices[firstV].position;
            glm::vec3 secondPos = m_Vertices[secondV].position;
            if (glm::distance(firstPos, secondPos) < THRESHOLD)
            {
                // add the pair idx to the vertices
                vertexPairLookup[firstV].insert(static_cast<unsigned int>(validPairs.size()));
                vertexPairLookup[secondV].insert(static_cast<unsigned int>(validPairs.size()));
                // add to valid pairs
                ValidPair newPair{}; newPair.vertOne = firstV; newPair.vertTwo = secondV; newPair.edge = false;
                validPairs.push_back(newPair);
            }
            // else, do nothing, not a valid pair
        }
    }

    // compute the new point and error associated for each valid pair

    // each pair should contain a few pieces of information
    // vertex 1, vertex 2, the new vertex position, the error after contraction, the quadric matrices for both 1 and 2, and the new vertex after contraction
    for (ValidPair validPair : validPairs)
    {
        glm::mat4 firstQuad = quadricLookup[validPair.vertOne];
        glm::mat4 secondQuad = quadricLookup[validPair.vertTwo];
        glm::mat4 Quad = firstQuad + secondQuad;

        glm::mat4 MatQ = {
            Quad[1][1], Quad[1][2], Quad[1][3], Quad[1][4],
            Quad[1][2], Quad[2][2], Quad[2][3], Quad[2][4],
            Quad[1][3], Quad[2][3], Quad[3][3], Quad[3][4],
            0,          0,          0,          1
        };
        if (glm::determinant(MatQ) != 0)
        {
            glm::vec4 newVPos = glm::inverse(MatQ) * glm::vec4{ 0, 0, 0, 1 };
            
            // 1x4 vector * 4x4 matrix * 4x1 vector yields a 1x1 matrix
            validPair.error = (newVPos * Quad * newVPos)[0];

            // change back to 3d coords from homogeneous coordinates
            validPair.newVert = glm::vec3(newVPos) / newVPos.w;
        }
        else
        {
            glm::vec4 end1 = { m_Vertices[validPair.vertOne].position, 1.0f };
            // 1x4 vector * 4x4 matrix * 4x1 vector yields a 1x1 matrix
            float end1Error = (end1 * Quad * end1)[0];

            glm::vec4 end2 = { m_Vertices[validPair.vertTwo].position, 1.0f };
            // 1x4 vector * 4x4 matrix * 4x1 vector yields a 1x1 matrix
            float end2Error = (end2 * Quad * end2)[0];

            glm::vec4 mid = (end1 + end2) / 2.0f;
            // 1x4 vector * 4x4 matrix * 4x1 vector yields a 1x1 matrix
            float midError = (mid * Quad * mid)[0];

            float minError = std::min({ end1Error, end2Error, midError });
            validPair.error = minError;
            if (minError == end1Error)
            {
                validPair.newVert = end1;
            }
            if (minError == end2Error)
            {
                validPair.newVert = end2;
            }
            if (minError == midError)
            {
                validPair.newVert = mid;
            }
        }
    }

    // create a min-heap to store all the valid pairs, ordered by error cost
    m_QuadricErrorHeap = std::priority_queue<ValidPair, std::vector<ValidPair>, CompareValidPairs>(validPairs.begin(), validPairs.end());

    // iteratively remove the validpair with the lowest cost, until numFaces == desiredCount
    unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());
    while (numFaces > desiredCount)
    {
        ValidPair leastCost = m_QuadricErrorHeap.top();
        m_QuadricErrorHeap.pop();

        // contract the current pair
        // TODO: need to replace the pair with the new vertex, change all neighbours to use the new vertex
        // change m_Vertices[leastCost.vertOne] = newV?
        m_Vertices[leastCost.vertOne].position = leastCost.newVert;
        m_Vertices[leastCost.vertTwo].position = leastCost.newVert;
        
        // if there are faces that use this current edge, remove it
        if (leastCost.edge)
        {
            // get edge index
            unsigned int edgeIdx = getEdgeIndex({ leastCost.vertOne, leastCost.vertTwo });

            for (auto it = m_Faces.begin(); it != m_Faces.end();)
            {
                FaceRecord face = *it;
                if (std::find(face.edgesIdx.begin(), face.edgesIdx.end(), edgeIdx) != face.edgesIdx.end())
                    it = m_Faces.erase(it); // Remove the element and update iterator
                else
                    it++;
            }
            numFaces = static_cast<unsigned int>(m_Faces.size());
        }


        // update the cost of all valid pairs involving the current pair
        //for (ValidPair validPair : validPairs)
        //{
        //    if ((validPair.vertOne == leastCost.vertOne) || (validPair.vertOne == leastCost.vertTwo))
        //        // update cost
        //    else if ((validPair.vertTwo == leastCost.vertOne) || (validPair.vertTwo == leastCost.vertTwo))
        //        // update cost

        //}
    }

    return GHOutputOBJ();
}
