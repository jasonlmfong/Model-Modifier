#include "Surface.h"


////////// helpers to build the Object //////////

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

        for (unsigned int i = 0; i < face.verticesIdx.size(); i++)
        {
            if (face.verticesIdx[i] < m_Vertices.size())
            {
                glm::vec3 vert = m_Vertices[face.verticesIdx[i]].position;
                vertsIdx.push_back(getVertIndex(vert, VertexPos, VertLookup));
            }
        }

        // Only add face if it has at least 3 unique vertices
        std::set<unsigned int> uniqueVerts(vertsIdx.begin(), vertsIdx.end());
        if (uniqueVerts.size() >= 3)
        {
            FaceIndices.push_back(vertsIdx);
            NumberPolygons[static_cast<unsigned int>(vertsIdx.size())] += 1;
        }
    }

    // build object
    Object Obj;
    Obj.m_Min = m_Min; Obj.m_Max = m_Max;
    Obj.m_VertexPos = VertexPos; Obj.m_FaceIndices = FaceIndices;
    Obj.m_NumPolygons = NumberPolygons;
    Obj.TriangulateFaces();

    return Obj;
}


////////// helpers for the GH algorithm //////////

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

// build the matrix for solving optimal vertex position
glm::mat4 Surface::BuildQuadricSolverMatrix(const glm::mat4& Quad)
{
    glm::mat4 MatQ;
    MatQ[0][0] = Quad[0][0]; MatQ[1][0] = Quad[1][0]; MatQ[2][0] = Quad[2][0]; MatQ[3][0] = Quad[3][0];
    MatQ[0][1] = Quad[0][1]; MatQ[1][1] = Quad[1][1]; MatQ[2][1] = Quad[2][1]; MatQ[3][1] = Quad[3][1];
    MatQ[0][2] = Quad[0][2]; MatQ[1][2] = Quad[1][2]; MatQ[2][2] = Quad[2][2]; MatQ[3][2] = Quad[3][2];
    MatQ[0][3] = 0;          MatQ[1][3] = 0;          MatQ[2][3] = 0;          MatQ[3][3] = 1;
    return MatQ;
}

// compute the optimal vertex position and error for a valid pair contraction
void Surface::ComputeOptimalVertexAndError(ValidPair& validPair, const glm::mat4& quadric1, const glm::mat4& quadric2)
{
    glm::mat4 Quad = quadric1 + quadric2;
    glm::mat4 MatQ = BuildQuadricSolverMatrix(Quad);

    if (glm::determinant(MatQ) != 0)
    {
        glm::vec4 newVPos = glm::inverse(MatQ) * glm::vec4{ 0, 0, 0, 1 };

        // calculate error: v^T * Q * v
        validPair.error = glm::dot(newVPos, Quad * newVPos);

        // change back to 3d coords from homogeneous coordinates
        validPair.newVert = glm::vec3(newVPos) / newVPos.w;
    }
    else
    {
        glm::vec4 end1 = { m_Vertices[validPair.vertOne].position, 1.0f };
        float end1Error = glm::dot(end1, Quad * end1);

        glm::vec4 end2 = { m_Vertices[validPair.vertTwo].position, 1.0f };
        float end2Error = glm::dot(end2, Quad * end2);

        glm::vec4 mid = (end1 + end2) / 2.0f;
        float midError = glm::dot(mid, Quad * mid);

        float minError = std::min({ end1Error, end2Error, midError });
        validPair.error = minError;
        if (minError == end1Error)
        {
            validPair.newVert = glm::vec3(end1) / end1.w;
        }
        else if (minError == end2Error)
        {
            validPair.newVert = glm::vec3(end2) / end2.w;
        }
        else if (minError == midError)
        {
            validPair.newVert = glm::vec3(mid) / mid.w;
        }
    }
}

// update adjacency indices after face removal, removing deleted indices and shifting remaining ones
void Surface::UpdateAdjacencyIndices(std::vector<unsigned int>& adjFaces, const std::vector<unsigned int>& removedFaceIndices)
{
    std::vector<unsigned int> newAdjFaces;
    for (unsigned int faceIdx : adjFaces)
    {
        // count how many removed faces have indices less than this one
        unsigned int offset = 0;
        for (unsigned int removedIdx : removedFaceIndices)
        {
            if (removedIdx < faceIdx)
            {
                offset++;
            }
            else if (removedIdx == faceIdx)
            {
                // this face was removed, skip it
                offset = UINT_MAX;
                break;
            }
        }

        if (offset != UINT_MAX)
        {
            newAdjFaces.push_back(faceIdx - offset);
        }
    }
    adjFaces = newAdjFaces;
}


////////// algorithms //////////

// Garland Heckbert simplification surface algorithm
Object Surface::QEM(unsigned int desiredCount)
{
    unsigned int numVertices = static_cast<unsigned int>(m_Vertices.size());

    // identify boundary edges (edges with only one adjacent face)
    std::set<unsigned int> boundaryEdges;
    for (unsigned int i = 0; i < m_Edges.size(); i++)
    {
        if (m_Edges[i].adjFacesIdx.size() == 1)
        {
            boundaryEdges.insert(i);
        }
    }

    // calculate quadric error for each vertex
    std::unordered_map<unsigned int, glm::mat4> quadricLookup;
    const float BOUNDARY_WEIGHT = 1000.0f; // large weight to preserve boundaries

    for (unsigned int i = 0; i < numVertices; i++)
    {
        glm::mat4 quadric = ComputeQuadric(m_Vertices[i]);

        // add penalty quadric for boundary vertices
        bool isBoundaryVertex = false;
        for (unsigned int edgeIdx : m_Vertices[i].adjEdgesIdx)
        {
            if (boundaryEdges.find(edgeIdx) != boundaryEdges.end())
            {
                isBoundaryVertex = true;

                // Create constraint plane perpendicular to the boundary edge
                EdgeRecord& edge = m_Edges[edgeIdx];
                glm::vec3 v1 = m_Vertices[edge.endPoint1Idx].position;
                glm::vec3 v2 = m_Vertices[edge.endPoint2Idx].position;
                glm::vec3 edgeDir = glm::normalize(v2 - v1);

                // for a boundary edge, create a perpendicular constraint with face normal of the adjacent face
                if (!edge.adjFacesIdx.empty() && edge.adjFacesIdx[0] < m_Faces.size())
                {
                    glm::vec3 faceNormal = ComputeFaceNormal(m_Faces[edge.adjFacesIdx[0]]);
                    glm::vec3 perpendicular = glm::normalize(glm::cross(edgeDir, faceNormal));
                    glm::vec4 constraintPlane{ perpendicular, -glm::dot(perpendicular, v1) };
                    quadric += BOUNDARY_WEIGHT * glm::outerProduct(constraintPlane, constraintPlane);
                }
            }
        }

        quadricLookup.insert({ i, quadric });
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
    for (ValidPair& validPair : validPairs)
    {
        ComputeOptimalVertexAndError(validPair, quadricLookup[validPair.vertOne], quadricLookup[validPair.vertTwo]);
    }

    // create a min-heap to store all the valid pairs, ordered by error cost
    m_QuadricErrorHeap = std::priority_queue<ValidPair, std::vector<ValidPair>, CompareValidPairs>(validPairs.begin(), validPairs.end());

    // track which pairs have been contracted to avoid processing stale duplicates
    std::set<std::pair<unsigned int, unsigned int>> contractedPairs;

    // track which vertices have been merged into others
    std::set<unsigned int> deletedVertices;

    // iteratively remove the validpair with the lowest cost, until numFaces == desiredCount
    unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());
    while (numFaces > desiredCount && !m_QuadricErrorHeap.empty())
    {
        ValidPair leastCost = m_QuadricErrorHeap.top();
        m_QuadricErrorHeap.pop();

        // skip if either vertex has been deleted/merged
        if (deletedVertices.find(leastCost.vertOne) != deletedVertices.end() ||
            deletedVertices.find(leastCost.vertTwo) != deletedVertices.end())
        {
            continue;
        }

        // skip if this pair has already been contracted
        auto pairKey = std::make_pair(
            std::min(leastCost.vertOne, leastCost.vertTwo),
            std::max(leastCost.vertOne, leastCost.vertTwo)
        );
        if (contractedPairs.find(pairKey) != contractedPairs.end())
        {
            continue;
        }

        // mark this pair as contracted
        contractedPairs.insert(pairKey);

        // mark vertTwo as deleted
        deletedVertices.insert(leastCost.vertTwo);

        // store original face normals BEFORE any modifications, used to compare later
        std::set<unsigned int> facesToUpdate;
        facesToUpdate.insert(m_Vertices[leastCost.vertTwo].adjFacesIdx.begin(), m_Vertices[leastCost.vertTwo].adjFacesIdx.end());
        facesToUpdate.insert(m_Vertices[leastCost.vertOne].adjFacesIdx.begin(), m_Vertices[leastCost.vertOne].adjFacesIdx.end());

        std::unordered_map<unsigned int, glm::vec3> originalNormals;
        for (unsigned int faceIdx : facesToUpdate)
        {
            if (faceIdx < m_Faces.size())
            {
                originalNormals[faceIdx] = ComputeFaceNormal(m_Faces[faceIdx]);
            }
        }

        // contract the current pair
        // move vertOne to the new position, and merge all references to vertTwo into vertOne
        m_Vertices[leastCost.vertOne].position = leastCost.newVert;

        // merge all faces, edges from vertTwo into vertOne
        for (unsigned int faceIdx : m_Vertices[leastCost.vertTwo].adjFacesIdx)
        {
            if (std::find(m_Vertices[leastCost.vertOne].adjFacesIdx.begin(), 
                         m_Vertices[leastCost.vertOne].adjFacesIdx.end(), faceIdx) 
                == m_Vertices[leastCost.vertOne].adjFacesIdx.end())
            {
                m_Vertices[leastCost.vertOne].adjFacesIdx.push_back(faceIdx);
            }
        }

        for (unsigned int edgeIdx : m_Vertices[leastCost.vertTwo].adjEdgesIdx)
        {
            if (std::find(m_Vertices[leastCost.vertOne].adjEdgesIdx.begin(), 
                         m_Vertices[leastCost.vertOne].adjEdgesIdx.end(), edgeIdx) 
                == m_Vertices[leastCost.vertOne].adjEdgesIdx.end())
            {
                m_Vertices[leastCost.vertOne].adjEdgesIdx.push_back(edgeIdx);
            }
        }

        // update all faces that reference vertTwo to reference vertOne instead
        // (facesToUpdate and originalNormals were already computed before vertex position change)
        for (unsigned int faceIdx : facesToUpdate)
        {
            if (faceIdx < m_Faces.size()) // Bounds check
            {
                FaceRecord& face = m_Faces[faceIdx];

                // check if this face contains BOTH vertices - if so, it will become degenerate
                bool containsVertOne = std::find(face.verticesIdx.begin(), face.verticesIdx.end(), leastCost.vertOne) != face.verticesIdx.end();
                bool containsVertTwo = std::find(face.verticesIdx.begin(), face.verticesIdx.end(), leastCost.vertTwo) != face.verticesIdx.end();

                // store original normal before any changes
                glm::vec3 originalNormal = originalNormals[faceIdx];

                // update vertex references
                for (unsigned int& vertIdx : face.verticesIdx)
                {
                    if (vertIdx == leastCost.vertTwo)
                    {
                        vertIdx = leastCost.vertOne;
                    }
                }

                // only check orientation if face originally contained only ONE of the two vertices
                // (faces with both vertices will become degenerate and be removed later)
                if (containsVertOne != containsVertTwo) // basically an XOR here
                {
                    // calculate new normal after vertex update
                    glm::vec3 newNormal = ComputeFaceNormal(face);

                    // validate that both normals are non-zero before comparing
                    float originalLength = glm::length(originalNormal);
                    float newLength = glm::length(newNormal);

                    if (originalLength > 1e-6f && newLength > 1e-6f)
                    {
                        // normalize for accurate dot product comparison
                        glm::vec3 origNormalized = originalNormal / originalLength;
                        glm::vec3 newNormalized = newNormal / newLength;

                        // if new normal is opposite to original, flip the face to preserve orientation
                        if (glm::dot(newNormalized, origNormalized) < 0.0f)
                        {
                            std::reverse(face.verticesIdx.begin(), face.verticesIdx.end());
                        }
                    }
                }
            }
        }

        // update all edges that reference vertTwo to reference vertOne instead
        for (unsigned int edgeIdx : m_Vertices[leastCost.vertTwo].adjEdgesIdx)
        {
            EdgeRecord& edge = m_Edges[edgeIdx];
            if (edge.endPoint1Idx == leastCost.vertTwo)
            {
                edge.endPoint1Idx = leastCost.vertOne;
            }
            if (edge.endPoint2Idx == leastCost.vertTwo)
            {
                edge.endPoint2Idx = leastCost.vertOne;
            }
        }

        // remove degenerate faces (after vertex updates are complete)
        std::vector<unsigned int> removedFaceIndices;

        for (unsigned int i = 0; i < m_Faces.size(); i++)
        {
            // check if face has duplicate vertices (degenerate after vertex merge)
            std::set<unsigned int> uniqueVerts(m_Faces[i].verticesIdx.begin(), m_Faces[i].verticesIdx.end());
            if (uniqueVerts.size() < m_Faces[i].verticesIdx.size())
            {
                removedFaceIndices.push_back(i);
            }
        }

        // remove faces in reverse order to maintain indices
        for (auto it = removedFaceIndices.rbegin(); it != removedFaceIndices.rend(); ++it)
        {
            m_Faces.erase(m_Faces.begin() + *it);
        }

        // update all vertex adjacency lists: remove deleted indices and shift remaining ones
        for (VertexRecord& vertex : m_Vertices)
        {
            UpdateAdjacencyIndices(vertex.adjFacesIdx, removedFaceIndices);
        }

        // update all edge adjacency lists similarly
        for (EdgeRecord& edge : m_Edges)
        {
            UpdateAdjacencyIndices(edge.adjFacesIdx, removedFaceIndices);
        }

        numFaces = static_cast<unsigned int>(m_Faces.size());

        // update the quadric for the merged vertex
        quadricLookup[leastCost.vertOne] = quadricLookup[leastCost.vertOne] + quadricLookup[leastCost.vertTwo];

        // update the cost of all valid pairs involving the current pair
        for (unsigned int pairIdx : vertexPairLookup[leastCost.vertOne])
        {
            ValidPair& validPair = validPairs[pairIdx];

            // skip if this pair was already contracted
            auto checkPair = std::make_pair(
                std::min(validPair.vertOne, validPair.vertTwo),
                std::max(validPair.vertOne, validPair.vertTwo)
            );
            if (contractedPairs.find(checkPair) != contractedPairs.end())
            {
                continue;
            }

            // determine which vertex in the pair is the one we need to update
            unsigned int otherVert = (validPair.vertOne == leastCost.vertOne) ? validPair.vertTwo : validPair.vertOne;

            // recalculate error for this pair
            ComputeOptimalVertexAndError(validPair, quadricLookup[leastCost.vertOne], quadricLookup[otherVert]);

            // push back to heap if neither vertex has been deleted
            if (deletedVertices.find(validPair.vertOne) == deletedVertices.end() &&
                deletedVertices.find(validPair.vertTwo) == deletedVertices.end())
            {
                m_QuadricErrorHeap.push(validPair);
            }
        }

        for (unsigned int pairIdx : vertexPairLookup[leastCost.vertTwo])
        {
            ValidPair& validPair = validPairs[pairIdx];

            // skip the pair we just contracted
            if ((validPair.vertOne == leastCost.vertOne && validPair.vertTwo == leastCost.vertTwo) ||
                (validPair.vertOne == leastCost.vertTwo && validPair.vertTwo == leastCost.vertOne))
            {
                continue;
            }

            // skip if this pair was already contracted
            auto checkPair = std::make_pair(
                std::min(validPair.vertOne, validPair.vertTwo),
                std::max(validPair.vertOne, validPair.vertTwo)
            );
            if (contractedPairs.find(checkPair) != contractedPairs.end())
            {
                continue;
            }

            // determine which vertex in the pair is the one we need to update
            unsigned int otherVert = (validPair.vertOne == leastCost.vertTwo) ? validPair.vertTwo : validPair.vertOne;

            // update the pair to reference vertOne instead of vertTwo
            if (validPair.vertOne == leastCost.vertTwo)
            {
                validPair.vertOne = leastCost.vertOne;
            }
            if (validPair.vertTwo == leastCost.vertTwo)
            {
                validPair.vertTwo = leastCost.vertOne;
            }

            // add this pair to vertOne's lookup
            vertexPairLookup[leastCost.vertOne].insert(pairIdx);

            // recalculate error for this pair
            ComputeOptimalVertexAndError(validPair, quadricLookup[leastCost.vertOne], quadricLookup[otherVert]);

            // push back to heap if neither vertex has been deleted
            if (deletedVertices.find(validPair.vertOne) == deletedVertices.end() &&
                deletedVertices.find(validPair.vertTwo) == deletedVertices.end())
            {
                m_QuadricErrorHeap.push(validPair);
            }
        }
    }

    return GHOutputOBJ();
}
