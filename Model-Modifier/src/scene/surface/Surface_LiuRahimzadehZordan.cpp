#include "Surface.h"


////////// helpers for the LRZ algorithm //////////

// compute line quadric by constructing quadric matrices from edges adjacent to vertex
// note this is not the same as the paper, we sum over adjacent edges instead of using vertex normal
// the proper implementation is below
glm::mat4 Surface::ComputeLineQuadric(VertexRecord v0)
{
    glm::mat4 lineQuadric{ 0.0f };
    glm::vec3 position = v0.position;

    // for each adjacent edge, create a line constraint
    for (unsigned int edgeIdx : v0.adjEdgesIdx)
    {
        if (edgeIdx >= m_Edges.size()) continue;

        EdgeRecord edge = m_Edges[edgeIdx];

        // get the other endpoint of the edge
        glm::vec3 otherPos;
        if (edge.endPoint1Idx < m_Vertices.size() && edge.endPoint2Idx < m_Vertices.size())
        {
            if (m_Vertices[edge.endPoint1Idx].position == position)
            {
                otherPos = m_Vertices[edge.endPoint2Idx].position;
            }
            else
            {
                otherPos = m_Vertices[edge.endPoint1Idx].position;
            }
        }
        else
        {
            continue;
        }

        // compute line direction
        glm::vec3 lineDir = glm::normalize(otherPos - position);

        // create perpendicular constraint planes for the line
        // we need two orthogonal planes that contain the line
        glm::vec3 perp1, perp2;

        // find first perpendicular vector
        if (std::abs(lineDir.x) < 0.9f)
        {
            perp1 = glm::normalize(glm::cross(lineDir, glm::vec3(1.0f, 0.0f, 0.0f)));
        }
        else
        {
            perp1 = glm::normalize(glm::cross(lineDir, glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        // second perpendicular is orthogonal to both line and first perpendicular
        perp2 = glm::normalize(glm::cross(lineDir, perp1));

        // create plane equations: the point should lie on the line
        // perpendicular constraint: n · (x - p) = 0 → n · x = n · p
        glm::vec4 plane1{ perp1, -glm::dot(perp1, position) };
        glm::vec4 plane2{ perp2, -glm::dot(perp2, position) };

        // add both perpendicular plane quadrics
        lineQuadric += glm::outerProduct(plane1, plane1);
        lineQuadric += glm::outerProduct(plane2, plane2);
    }

    return lineQuadric;
}

// proper implementation of line quadric
// glm::mat4 Surface::ComputeLineQuadric(VertexRecord v0)
// {
//     glm::mat4 lineQuadric{ 0.0f };
//     glm::vec3 position = v0.position;

//     // Get (area-weighted) vertex normal
//     glm::vec3 vertNormal = glm::normalize(ComputeVertexNormal(v0));

//     // create perpendicular constraint planes for the line
//     // we need two orthogonal planes that contain the line
//     glm::vec3 perp1, perp2;

//     // find first perpendicular vector
//     if (std::abs(vertNormal.x) < 0.9f)
//     {
//         perp1 = glm::normalize(glm::cross(vertNormal, glm::vec3(1.0f, 0.0f, 0.0f)));
//     }
//     else
//     {
//         perp1 = glm::normalize(glm::cross(vertNormal, glm::vec3(0.0f, 1.0f, 0.0f)));
//     }

//     // second perpendicular is orthogonal to both line and first perpendicular
//     perp2 = glm::normalize(glm::cross(vertNormal, perp1));

//     // create plane equations: the point should lie on the line
//     // perpendicular constraint: n · (x - p) = 0 → n · x = n · p
//     glm::vec4 plane1{ perp1, -glm::dot(perp1, position) };
//     glm::vec4 plane2{ perp2, -glm::dot(perp2, position) };

//     // add both perpendicular plane quadrics
//     lineQuadric += glm::outerProduct(plane1, plane1);
//     lineQuadric += glm::outerProduct(plane2, plane2);

//     return lineQuadric;
// }

glm::mat4 Surface::ComputeWeightedQuadric(const glm::mat4& planeQuadric, const glm::mat4& lineQuadric, float alpha)
{
    return planeQuadric + alpha * lineQuadric;
}


////////// algorithms //////////

// Liu Rahimzadeh Zordan QEM simplification with line quadric constraints
Object Surface::LineQEM(unsigned int desiredCount, float alpha)
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

    // calculate both point and line quadrics for each vertex
    std::unordered_map<unsigned int, glm::mat4> planeQuadricLookup;
    std::unordered_map<unsigned int, glm::mat4> lineQuadricLookup;
    const float BOUNDARY_WEIGHT = 1000.0f; // large weight to preserve boundaries

    for (unsigned int i = 0; i < numVertices; i++)
    {
        glm::mat4 planeQuadric = ComputePlaneQuadric(m_Vertices[i]);
        glm::mat4 lineQuadric = ComputeLineQuadric(m_Vertices[i]);

        // add penalty quadric for boundary vertices to point quadric
        for (unsigned int edgeIdx : m_Vertices[i].adjEdgesIdx)
        {
            if (boundaryEdges.find(edgeIdx) != boundaryEdges.end())
            {
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
                    planeQuadric += BOUNDARY_WEIGHT * glm::outerProduct(constraintPlane, constraintPlane);
                }
            }
        }

        planeQuadricLookup.insert({ i, planeQuadric });
        lineQuadricLookup.insert({ i, lineQuadric });
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
                    ValidPair newPair{}; 
                    newPair.vertOne = firstV; 
                    newPair.vertTwo = secondV; 
                    newPair.edge = true;
                    newPair.alpha = alpha;
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
                ValidPair newPair{}; 
                newPair.vertOne = firstV; 
                newPair.vertTwo = secondV; 
                newPair.edge = false;
                newPair.alpha = alpha;
                validPairs.push_back(newPair);
            }
            // else, do nothing, not a valid pair
        }
    }

    // compute the new point and error associated for each valid pair
    for (ValidPair& validPair : validPairs)
    {
        ComputeOptimalVertexAndError(
            validPair,
            ComputeWeightedQuadric(planeQuadricLookup[validPair.vertOne], lineQuadricLookup[validPair.vertOne], alpha),
            ComputeWeightedQuadric(planeQuadricLookup[validPair.vertTwo], lineQuadricLookup[validPair.vertTwo], alpha)
        );
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
        for (unsigned int faceIdx : facesToUpdate)
        {
            if (faceIdx < m_Faces.size())
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
                if (containsVertOne != containsVertTwo)
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

        // remove degenerate faces
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

        // update all vertex adjacency lists
        for (VertexRecord& vertex : m_Vertices)
        {
            UpdateAdjacencyIndices(vertex.adjFacesIdx, removedFaceIndices);
        }

        // update all edge adjacency lists
        for (EdgeRecord& edge : m_Edges)
        {
            UpdateAdjacencyIndices(edge.adjFacesIdx, removedFaceIndices);
        }

        numFaces = static_cast<unsigned int>(m_Faces.size());

        // update both point and line quadrics for the merged vertex
        planeQuadricLookup[leastCost.vertOne] = planeQuadricLookup[leastCost.vertOne] + planeQuadricLookup[leastCost.vertTwo];
        lineQuadricLookup[leastCost.vertOne] = lineQuadricLookup[leastCost.vertOne] + lineQuadricLookup[leastCost.vertTwo];

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
            ComputeOptimalVertexAndError(
                validPair,
                ComputeWeightedQuadric(planeQuadricLookup[leastCost.vertOne], lineQuadricLookup[leastCost.vertOne], alpha),
                ComputeWeightedQuadric(planeQuadricLookup[otherVert], lineQuadricLookup[otherVert], alpha)
            );

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
            ComputeOptimalVertexAndError(
                validPair,
                ComputeWeightedQuadric(planeQuadricLookup[leastCost.vertOne], lineQuadricLookup[leastCost.vertOne], alpha),
                ComputeWeightedQuadric(planeQuadricLookup[otherVert], lineQuadricLookup[otherVert], alpha)
            );

            // push back to heap if neither vertex has been deleted
            if (deletedVertices.find(validPair.vertOne) == deletedVertices.end() &&
                deletedVertices.find(validPair.vertTwo) == deletedVertices.end())
            {
                m_QuadricErrorHeap.push(validPair);
            }
        }
    }

    return QEMOutputOBJ();
}
