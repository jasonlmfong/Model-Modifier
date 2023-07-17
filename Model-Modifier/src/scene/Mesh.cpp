#include "Mesh.h"

Mesh::Mesh(Object obj, int shading)
    : m_Object(obj), m_ShadingType(shading)
{
    BuildFaceNormals();
    BuildVerticesIndices();
}

Mesh::~Mesh()
{
    Destroy();
}

void Mesh::BuildFaceNormals()
{
    // build face normal vectors from obj
    m_FaceNormals.resize(m_Object.m_FaceIndices.size());
    for (int i = 0; i < m_Object.m_FaceIndices.size(); i++)
    {
        unsigned int ia = m_Object.m_FaceIndices[i][0];
        unsigned int ib = m_Object.m_FaceIndices[i][1];
        unsigned int ic = m_Object.m_FaceIndices[i][2];
        glm::vec3 normal = glm::normalize(
            glm::cross(
                m_Object.m_VertexPos[ib] - m_Object.m_VertexPos[ia],
                m_Object.m_VertexPos[ic] - m_Object.m_VertexPos[ia]
            )
        );
        m_FaceNormals[i] = normal;
    }
}

void Mesh::BuildVerticesIndices()
{
    // build output items to OpenGL
    if (m_ShadingType == FLAT) // flat shading
    {
        // build out the VBO with x,y,z coords of vertices, and normal vectors
        m_OutNumVert = 2 * 3 * 3 * m_Object.m_FaceIndices.size();
        m_OutVertices = new float[m_OutNumVert] {};
        for (int i = 0; i < m_Object.m_FaceIndices.size(); i++)
        {
            for (int j = 0; j < 3; j++)
            {
                // ith face, jth corner, xyz coordinates and normals
                m_OutVertices[18 * i + 6 * j + 0] = m_Object.m_VertexPos[m_Object.m_FaceIndices[i][j]].x;
                m_OutVertices[18 * i + 6 * j + 1] = m_Object.m_VertexPos[m_Object.m_FaceIndices[i][j]].y;
                m_OutVertices[18 * i + 6 * j + 2] = m_Object.m_VertexPos[m_Object.m_FaceIndices[i][j]].z;

                m_OutVertices[18 * i + 6 * j + 3] = m_FaceNormals[i].x;
                m_OutVertices[18 * i + 6 * j + 4] = m_FaceNormals[i].y;
                m_OutVertices[18 * i + 6 * j + 5] = m_FaceNormals[i].z;
            }
        }

        // build out IBO indices
        m_OutNumIdx = 3 * 3 * m_Object.m_FaceIndices.size();
        m_OutIndices = new unsigned int[m_OutNumIdx];
        for (int i = 0; i < m_OutNumIdx; i++)
        {
            m_OutIndices[i] = i;
        }
    }
    else if (m_ShadingType == MIXED) // mixed shading
    {
        // get vertex-face connectivity: get all faces that touch the a given vertex
        std::unordered_map<unsigned int, std::vector<unsigned int>> vertAdjFaces(m_Object.m_VertexPos.size());
        for (int faceIdx = 0; faceIdx < m_Object.m_FaceIndices.size(); faceIdx++)
        {
            std::vector<unsigned int> faceVertIdx = m_Object.m_FaceIndices[faceIdx];
            // add face index to the connecting vertices
            for (int i = 0; i < 3; i++)
            {
                vertAdjFaces[faceVertIdx[i]].push_back(faceIdx);
            }
        }

        // build mixed vertex normals by first getting current face normal, then average adjacent normals
        std::vector<std::vector<glm::vec3>> cornerVertexNormals(m_Object.m_FaceIndices.size());
        for (unsigned int currFace = 0; currFace < m_Object.m_FaceIndices.size(); currFace++)
        {
            // get face normal
            glm::vec3 currFaceNormal = m_FaceNormals[currFace];
            std::vector<glm::vec3> faceCornerNormals;

            for (unsigned int currCorner = 0; currCorner < m_Object.m_FaceIndices[currFace].size(); currCorner++)
            {
                // corner normal to be stored
                glm::vec3 currCornerNormal {0};

                // go through all neighbour faces (including current face)
                for (unsigned int adjFace : vertAdjFaces[m_Object.m_FaceIndices[currFace][currCorner]])
                {
                    glm::vec3 adjFaceNormal = m_FaceNormals[adjFace];
                    // if adjacent face is "close" to current face, then add the adj face normal to current corner normal
                    if (glm::dot(currFaceNormal, adjFaceNormal) > 0.9f) // threshold set to 0.9f here
                    {
                        currCornerNormal += adjFaceNormal;
                    }
                }
                faceCornerNormals.push_back(glm::normalize(currCornerNormal));
            }
            cornerVertexNormals[currFace] = faceCornerNormals;
        }

        // build out the VBO with x,y,z coords of vertices, and normal vectors
        m_OutNumVert = 2 * 3 * 3 * m_Object.m_FaceIndices.size();
        m_OutVertices = new float[m_OutNumVert] {};
        for (int i = 0; i < m_Object.m_FaceIndices.size(); i++)
        {
            for (int j = 0; j < 3; j++)
            {
                // ith face, jth corner, xyz coordinates and normals
                m_OutVertices[18 * i + 6 * j + 0] = m_Object.m_VertexPos[m_Object.m_FaceIndices[i][j]].x;
                m_OutVertices[18 * i + 6 * j + 1] = m_Object.m_VertexPos[m_Object.m_FaceIndices[i][j]].y;
                m_OutVertices[18 * i + 6 * j + 2] = m_Object.m_VertexPos[m_Object.m_FaceIndices[i][j]].z;

                m_OutVertices[18 * i + 6 * j + 3] = cornerVertexNormals[i][j].x;
                m_OutVertices[18 * i + 6 * j + 4] = cornerVertexNormals[i][j].y;
                m_OutVertices[18 * i + 6 * j + 5] = cornerVertexNormals[i][j].z;
            }
        }

        // build out IBO indices
        m_OutNumIdx = 3 * 3 * m_Object.m_FaceIndices.size();
        m_OutIndices = new unsigned int[m_OutNumIdx];
        for (int i = 0; i < m_OutNumIdx; i++)
        {
            m_OutIndices[i] = i;
        }
    }
    else if (m_ShadingType == SMOOTH) // smooth shading
    {
        // get vertex-face connectivity: get all faces that touch the a given vertex
        std::unordered_map<unsigned int, std::vector<unsigned int>> vertAdjFaces(m_Object.m_VertexPos.size());
        for (int faceIdx = 0; faceIdx < m_Object.m_FaceIndices.size(); faceIdx++)
        {
            std::vector<unsigned int> faceVertIdx = m_Object.m_FaceIndices[faceIdx];
            // add face index to the connecting vertices
            for (int i = 0; i < 3; i++)
            {
                vertAdjFaces[faceVertIdx[i]].push_back(faceIdx);
            }
        }

        // build smooth vertex normals by average neighbouring faces normals
        std::vector<glm::vec3> smoothVertexNormals(m_Object.m_VertexPos.size());
        for (int i = 0; i < m_Object.m_VertexPos.size(); i++)
        {
            glm::vec3 currPos = m_Object.m_VertexPos[i];
            glm::vec3 currVertNormal = glm::vec3(0, 0, 0);

            // summ through each neighbouring face
            for (unsigned int adjFace : vertAdjFaces[i])
            {
                currVertNormal += m_FaceNormals[adjFace];
            }

            smoothVertexNormals[i] = glm::normalize(currVertNormal);
        }

        // build out the VBO with x,y,z coords of vertices, and normal vectors
        m_OutNumVert = 2 * 3 * m_Object.m_VertexPos.size();
        m_OutVertices = new float[m_OutNumVert] {};
        for (int i = 0; i < m_Object.m_VertexPos.size(); i++)
        {
            // x value of the vertex
            m_OutVertices[6 * i + 0] = m_Object.m_VertexPos[i].x;
            // y value of the vertex
            m_OutVertices[6 * i + 1] = m_Object.m_VertexPos[i].y;
            // z value of the vertex
            m_OutVertices[6 * i + 2] = m_Object.m_VertexPos[i].z;

            // x value of the normal
            m_OutVertices[6 * i + 3] = smoothVertexNormals[i].x;
            // y value of the normal
            m_OutVertices[6 * i + 4] = smoothVertexNormals[i].y;
            // z value of the normal
            m_OutVertices[6 * i + 5] = smoothVertexNormals[i].z;
        }

        // build out IBO indices
        m_OutNumIdx = 3 * m_Object.m_FaceIndices.size();
        m_OutIndices = new unsigned int[m_OutNumIdx];
        for (int i = 0; i < m_Object.m_FaceIndices.size(); i++)
        {
            m_OutIndices[3 * i + 0] = m_Object.m_FaceIndices[i][0];
            m_OutIndices[3 * i + 1] = m_Object.m_FaceIndices[i][1];
            m_OutIndices[3 * i + 2] = m_Object.m_FaceIndices[i][2];
        }
    }
}

void Mesh::Destroy()
{
    m_FaceNormals.clear();

    delete[] m_OutVertices;
    delete[] m_OutIndices;
}

void Mesh::Rebuild()
{
    Destroy();
    BuildFaceNormals();
    BuildVerticesIndices();
}

void Mesh::Rebuild(Object obj)
{
    m_Object = obj;
    Rebuild();
}

void Mesh::Rebuild(int shading)
{
    m_ShadingType = shading;
    Rebuild();
}
