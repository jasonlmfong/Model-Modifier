#include "Mesh.h"

Mesh::Mesh(Object obj, int shading)
    : m_Object(obj), m_ShadingType(shading)
{
    BuildVerticesIndices();
}

Mesh::~Mesh()
{
    Destroy();
}

void Mesh::BuildVerticesIndices()
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
    else if (m_ShadingType == SMOOTH) // smooth shading
    {
        // build smooth vertex normals by average neighbouring faces normals
        std::vector<glm::vec3> smoothVertexNormals(m_Object.m_VertexPos.size());

        for (int i = 0; i < m_Object.m_VertexPos.size(); i++)
        {
            glm::vec3 currVertNormal = glm::vec3(0, 0, 0);

            for (int face = 0; face < m_Object.m_FaceIndices.size(); face++)
            {
                if (i == m_Object.m_FaceIndices[face][0] || i == m_Object.m_FaceIndices[face][1] || i == m_Object.m_FaceIndices[face][2])
                {
                    currVertNormal += m_FaceNormals[face];
                }
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

void Mesh::BuildVerticesIndices(Object obj)
{
    m_Object = obj;
    BuildVerticesIndices();
}

void Mesh::BuildVerticesIndices(int shading)
{
    m_ShadingType = shading;
    BuildVerticesIndices();
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
