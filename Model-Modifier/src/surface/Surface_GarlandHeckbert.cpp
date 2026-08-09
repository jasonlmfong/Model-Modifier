#include "Surface.h"

#include <algorithm>
#include <set>
#include <queue>

using namespace SurfaceDetail;

////////// algorithms //////////

// Garland Heckbert simplification surface algorithm
void SimplifyGarlandHeckbert(Mesh& mesh, unsigned int desiredCount)
{
    mesh.BuildTopology();

    unsigned int numVertices = static_cast<unsigned int>(mesh.m_TopoVertices.size());

    // identify boundary edges (edges with only one adjacent face)
    std::set<unsigned int> boundaryEdges;
    for (unsigned int i = 0; i < mesh.m_TopoEdges.size(); i++)
    {
        if (mesh.m_TopoEdges[i].adjFacesIdx.size() == 1)
        {
            boundaryEdges.insert(i);
        }
    }

    // calculate quadric error for each vertex
    std::unordered_map<unsigned int, glm::mat4> quadricLookup;
    const float BOUNDARY_WEIGHT = 1000.0f; // large weight to preserve boundaries

    for (unsigned int i = 0; i < numVertices; i++)
    {
        glm::mat4 quadric = ComputePlaneQuadric(mesh, mesh.m_TopoVertices[i]);

        // add penalty quadric for boundary vertices
        bool isBoundaryVertex = false;
        for (unsigned int edgeIdx : mesh.m_TopoVertices[i].adjEdgesIdx)
        {
            if (boundaryEdges.find(edgeIdx) != boundaryEdges.end())
            {
                isBoundaryVertex = true;

                // Create constraint plane perpendicular to the boundary edge
                EdgeRecord& edge = mesh.m_TopoEdges[edgeIdx];
                glm::vec3 v1 = mesh.m_TopoVertices[edge.endPoint1Idx].position;
                glm::vec3 v2 = mesh.m_TopoVertices[edge.endPoint2Idx].position;
                glm::vec3 edgeDir = glm::normalize(v2 - v1);

                // for a boundary edge, create a perpendicular constraint with face normal of the adjacent face
                if (!edge.adjFacesIdx.empty() && edge.adjFacesIdx[0] < mesh.m_TopoFaces.size())
                {
                    glm::vec3 faceNormal = mesh.ComputeFaceNormal(mesh.m_TopoFaces[edge.adjFacesIdx[0]]);
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
            auto searchx = mesh.m_EdgeIdxLookup.find(firstV);
            if (searchx != mesh.m_EdgeIdxLookup.end())
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
            glm::vec3 firstPos = mesh.m_TopoVertices[firstV].position;
            glm::vec3 secondPos = mesh.m_TopoVertices[secondV].position;
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
        ComputeOptimalVertexAndError(mesh, validPair, quadricLookup[validPair.vertOne], quadricLookup[validPair.vertTwo]);
    }

    // create a min-heap to store all the valid pairs, ordered by error cost
    std::priority_queue<ValidPair, std::vector<ValidPair>, CompareValidPairs> quadricErrorHeap(validPairs.begin(), validPairs.end());

    // track which pairs have been contracted to avoid processing stale duplicates
    std::set<std::pair<unsigned int, unsigned int>> contractedPairs;

    // track which vertices have been merged into others
    std::set<unsigned int> deletedVertices;

    // iteratively remove the validpair with the lowest cost, until numFaces == desiredCount
    unsigned int numFaces = static_cast<unsigned int>(mesh.m_TopoFaces.size());
    while (numFaces > desiredCount && !quadricErrorHeap.empty())
    {
        ValidPair leastCost = quadricErrorHeap.top();
        quadricErrorHeap.pop();

        // skip if either vertex has been deleted/merged
        if (deletedVertices.find(leastCost.vertOne) != deletedVertices.end() ||
            deletedVertices.find(leastCost.vertTwo) != deletedVertices.end())
            continue;

        // skip if this pair has already been contracted
        auto pairKey = std::make_pair(
            std::min(leastCost.vertOne, leastCost.vertTwo),
            std::max(leastCost.vertOne, leastCost.vertTwo));
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
        facesToUpdate.insert(mesh.m_TopoVertices[leastCost.vertTwo].adjFacesIdx.begin(), mesh.m_TopoVertices[leastCost.vertTwo].adjFacesIdx.end());
        facesToUpdate.insert(mesh.m_TopoVertices[leastCost.vertOne].adjFacesIdx.begin(), mesh.m_TopoVertices[leastCost.vertOne].adjFacesIdx.end());

        std::unordered_map<unsigned int, glm::vec3> originalNormals;
        for (unsigned int faceIdx : facesToUpdate)
        {
            if (faceIdx < mesh.m_TopoFaces.size())
            {
                originalNormals[faceIdx] = mesh.ComputeFaceNormal(mesh.m_TopoFaces[faceIdx]);
            }
        }

        // contract the current pair
        // move vertOne to the new position, and merge all references to vertTwo into vertOne
        mesh.m_TopoVertices[leastCost.vertOne].position = leastCost.newVert;

        // merge all faces, edges from vertTwo into vertOne
        for (unsigned int faceIdx : mesh.m_TopoVertices[leastCost.vertTwo].adjFacesIdx)
        {
            if (std::find(mesh.m_TopoVertices[leastCost.vertOne].adjFacesIdx.begin(),
                         mesh.m_TopoVertices[leastCost.vertOne].adjFacesIdx.end(), faceIdx)
                == mesh.m_TopoVertices[leastCost.vertOne].adjFacesIdx.end())
            {
                mesh.m_TopoVertices[leastCost.vertOne].adjFacesIdx.push_back(faceIdx);
            }
        }

        for (unsigned int edgeIdx : mesh.m_TopoVertices[leastCost.vertTwo].adjEdgesIdx)
        {
            if (std::find(mesh.m_TopoVertices[leastCost.vertOne].adjEdgesIdx.begin(),
                         mesh.m_TopoVertices[leastCost.vertOne].adjEdgesIdx.end(), edgeIdx)
                == mesh.m_TopoVertices[leastCost.vertOne].adjEdgesIdx.end())
            {
                mesh.m_TopoVertices[leastCost.vertOne].adjEdgesIdx.push_back(edgeIdx);
            }
        }

        // update all faces that reference vertTwo to reference vertOne instead
        // (facesToUpdate and originalNormals were already computed before vertex position change)
        for (unsigned int faceIdx : facesToUpdate)
        {
            if (faceIdx < mesh.m_TopoFaces.size()) // Bounds check
            {
                FaceRecord& face = mesh.m_TopoFaces[faceIdx];

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
                    glm::vec3 newNormal = mesh.ComputeFaceNormal(face);

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
        for (unsigned int edgeIdx : mesh.m_TopoVertices[leastCost.vertTwo].adjEdgesIdx)
        {
            EdgeRecord& edge = mesh.m_TopoEdges[edgeIdx];
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

        for (unsigned int i = 0; i < mesh.m_TopoFaces.size(); i++)
        {
            // check if face has duplicate vertices (degenerate after vertex merge)
            std::set<unsigned int> uniqueVerts(mesh.m_TopoFaces[i].verticesIdx.begin(), mesh.m_TopoFaces[i].verticesIdx.end());
            if (uniqueVerts.size() < mesh.m_TopoFaces[i].verticesIdx.size())
            {
                removedFaceIndices.push_back(i);
            }
        }

        // remove faces in reverse order to maintain indices
        for (auto it = removedFaceIndices.rbegin(); it != removedFaceIndices.rend(); ++it)
        {
            mesh.m_TopoFaces.erase(mesh.m_TopoFaces.begin() + *it);
        }

        // update all vertex adjacency lists: remove deleted indices and shift remaining ones
        for (VertexRecord& vertex : mesh.m_TopoVertices)
        {
            UpdateAdjacencyIndices(vertex.adjFacesIdx, removedFaceIndices);
        }

        // update all edge adjacency lists similarly
        for (EdgeRecord& edge : mesh.m_TopoEdges)
        {
            UpdateAdjacencyIndices(edge.adjFacesIdx, removedFaceIndices);
        }

        numFaces = static_cast<unsigned int>(mesh.m_TopoFaces.size());

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
            ComputeOptimalVertexAndError(mesh, validPair, quadricLookup[leastCost.vertOne], quadricLookup[otherVert]);

            // push back to heap if neither vertex has been deleted
            if (deletedVertices.find(validPair.vertOne) == deletedVertices.end() &&
                deletedVertices.find(validPair.vertTwo) == deletedVertices.end())
            {
                quadricErrorHeap.push(validPair);
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
            ComputeOptimalVertexAndError(mesh, validPair, quadricLookup[leastCost.vertOne], quadricLookup[otherVert]);

            // push back to heap if neither vertex has been deleted
            if (deletedVertices.find(validPair.vertOne) == deletedVertices.end() &&
                deletedVertices.find(validPair.vertTwo) == deletedVertices.end())
            {
                quadricErrorHeap.push(validPair);
            }
        }
    }

    WriteTopologyToMesh(mesh);
}
