#include "Mesh.h"

Mesh::Mesh(const char* filename)
{
    loadOBJ(filename);
}

Mesh::~Mesh()
{
    Destroy();
}

void Mesh::loadOBJ(const char* filename)
{
    std::ifstream in(filename);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << std::endl; 
        exit(1);
    }

    std::string line;
    while (getline(in, line))
    {
        if (line.substr(0, 2) == "v ")
        {
            std::istringstream s(line.substr(2));
            glm::vec3 v; s >> v.x; s >> v.y; s >> v.z;
            m_VertexPos.push_back(v);
        }
        else if (line.substr(0, 2) == "f ")
        {
            std::istringstream s(line.substr(2));
            unsigned int a, b, c;
            s >> a; s >> b; s >> c;
            a--; b--; c--;
            std::vector<unsigned int> faceIndices;
            faceIndices.push_back(a); faceIndices.push_back(b); faceIndices.push_back(c);
            m_Elements.push_back(faceIndices);
        }
    }

    m_FaceNormals.resize(m_Elements.size());
    for (int i = 0; i < m_Elements.size(); i++)
    {
        unsigned int ia = m_Elements[i][0];
        unsigned int ib = m_Elements[i][1];
        unsigned int ic = m_Elements[i][2];
        glm::vec3 normal = glm::normalize(
            glm::cross(
                m_VertexPos[ib] - m_VertexPos[ia],
                m_VertexPos[ic] - m_VertexPos[ia]
            )
        );
        m_FaceNormals[i] = normal;
    }

    // TODO: USE VN FOR VERTEX NORMALS
    // TODO: IMPLEMENT "PER-CORNER" NORMALS
    m_VertexNormals.resize(m_VertexPos.size());
    for (int i = 0; i < m_VertexPos.size(); i++)
    {
        glm::vec3 currVertNormal = glm::vec3(0, 0, 0);

        for (int face = 0; face < m_Elements.size(); face++)
        {
            if (i == m_Elements[face][0] || i == m_Elements[face][1] || i == m_Elements[face][2])
            {
                currVertNormal += m_FaceNormals[face];
            }
        }

        m_VertexNormals[i] = glm::normalize(currVertNormal);
    }


    // 3D pos and normal of each vertex
    m_Vertices = new float[2 * 3 * m_VertexPos.size()] {};
    for (int i = 0; i < m_VertexPos.size(); i++)
    {
        // x value of the vertex
        m_Vertices[6 * i + 0] = m_VertexPos[i].x;
        // y value of the vertex
        m_Vertices[6 * i + 1] = m_VertexPos[i].y;
        // z value of the vertex
        m_Vertices[6 * i + 2] = m_VertexPos[i].z;

        // x value of the normal
        m_Vertices[6 * i + 3] = m_VertexNormals[i].x;
        // y value of the normal
        m_Vertices[6 * i + 4] = m_VertexNormals[i].y;
        // z value of the normal
        m_Vertices[6 * i + 5] = m_VertexNormals[i].z;
    }

    m_Indices = new unsigned int[3 * m_Elements.size()];
    for (int i = 0; i < m_Elements.size(); i++)
    {
        m_Indices[3 * i + 0] = m_Elements[i][0];
        m_Indices[3 * i + 1] = m_Elements[i][1];
        m_Indices[3 * i + 2] = m_Elements[i][2];
    }
}

void Mesh::Destroy()
{
    m_VertexPos.clear();
    delete[] m_Indices;
    m_FaceNormals.clear();
    m_VertexNormals.clear();
    delete[] m_Vertices;
}
