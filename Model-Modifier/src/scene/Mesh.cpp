#include "Mesh.h"

Mesh::Mesh(const char* filename)
{
    loadOBJ(filename);
}

Mesh::~Mesh()
{
    Destroy();
}

void customSplit(std::string str, char separator, std::vector<std::string> &strings) {
    int startIndex = 0, endIndex = 0;
    for (int i = 0; i <= str.size(); i++) 
    {
        // If we reached the end of the word or the end of the input.
        if (str[i] == separator || i == str.size()) 
        {
            endIndex = i;
            std::string temp;
            temp.append(str, startIndex, endIndex - startIndex);
            strings.push_back(temp);
            startIndex = endIndex + 1;
        }
    }
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
            std::string s(line.substr(2));
            std::vector<std::string> face;
            customSplit(s, ' ', face);
            std::vector<unsigned int> faceIndices;
            for (std::string corner : face) 
            {
                if (corner == "")
                    continue;
                std::vector<std::string> vertex;
                customSplit(corner, '/', vertex);
                // take the first position, regardless of the format (f v v v, f v/vt v/vt v/vt, f v/vt/vn v/vt/vn v/vt/vn)
                unsigned int vertIdx = stoul(vertex[0]); 
                vertIdx--; // change from obj index to c++ index
                
                faceIndices.push_back(vertIdx);
            }
            m_FaceIndices.push_back(faceIndices);
        }
    }

    m_FaceNormals.resize(m_FaceIndices.size());
    for (int i = 0; i < m_FaceIndices.size(); i++)
    {
        unsigned int ia = m_FaceIndices[i][0];
        unsigned int ib = m_FaceIndices[i][1];
        unsigned int ic = m_FaceIndices[i][2];
        glm::vec3 normal = glm::normalize(
            glm::cross(
                m_VertexPos[ib] - m_VertexPos[ia],
                m_VertexPos[ic] - m_VertexPos[ia]
            )
        );
        m_FaceNormals[i] = normal;
    }

    m_SmoothVertexNormals.resize(m_VertexPos.size());
    for (int i = 0; i < m_VertexPos.size(); i++)
    {
        glm::vec3 currVertNormal = glm::vec3(0, 0, 0);

        for (int face = 0; face < m_FaceIndices.size(); face++)
        {
            if (i == m_FaceIndices[face][0] || i == m_FaceIndices[face][1] || i == m_FaceIndices[face][2])
            {
                currVertNormal += m_FaceNormals[face];
            }
        }

        m_SmoothVertexNormals[i] = glm::normalize(currVertNormal);
    }

    ////////// OUTPUTS TO OPENGL /////////
    // 3D pos and normal of each vertex
    m_FlatNumVert = 2 * 3 * 3 * m_FaceIndices.size();
    m_FlatVertices = new float[m_FlatNumVert] {};
    for (int i = 0; i < m_FaceIndices.size(); i++) 
    {
        for (int j = 0; j < 3; j++)
        {
            // ith face, jth corner, xyz coordinates and normals
            m_FlatVertices[18 * i + 6 * j + 0] = m_VertexPos[m_FaceIndices[i][j]].x;
            m_FlatVertices[18 * i + 6 * j + 1] = m_VertexPos[m_FaceIndices[i][j]].y;
            m_FlatVertices[18 * i + 6 * j + 2] = m_VertexPos[m_FaceIndices[i][j]].z;

            m_FlatVertices[18 * i + 6 * j + 3] = m_FaceNormals[i].x;
            m_FlatVertices[18 * i + 6 * j + 4] = m_FaceNormals[i].y;
            m_FlatVertices[18 * i + 6 * j + 5] = m_FaceNormals[i].z;
        }
    }


    m_FlatNumIdx = 3 * 3 * m_FaceIndices.size();
    m_FlatIndices = new unsigned int[m_FlatNumIdx];
    for (int i = 0; i < m_FlatNumIdx; i++)
    {
        m_FlatIndices[i] = i;
    }

    ////////// OUTPUTS TO OPENGL /////////
    // 3D pos and normal of each vertex
    m_SmoothNumVert = 2 * 3 * m_VertexPos.size();
    m_SmoothVertices = new float[m_SmoothNumVert] {};
    for (int i = 0; i < m_VertexPos.size(); i++)
    {
        // x value of the vertex
        m_SmoothVertices[6 * i + 0] = m_VertexPos[i].x;
        // y value of the vertex
        m_SmoothVertices[6 * i + 1] = m_VertexPos[i].y;
        // z value of the vertex
        m_SmoothVertices[6 * i + 2] = m_VertexPos[i].z;

        // x value of the normal
        m_SmoothVertices[6 * i + 3] = m_SmoothVertexNormals[i].x;
        // y value of the normal
        m_SmoothVertices[6 * i + 4] = m_SmoothVertexNormals[i].y;
        // z value of the normal
        m_SmoothVertices[6 * i + 5] = m_SmoothVertexNormals[i].z;
    }

    m_SmoothNumIdx = 3 * m_FaceIndices.size();
    m_SmoothIndices = new unsigned int[m_SmoothNumIdx];
    for (int i = 0; i < m_FaceIndices.size(); i++)
    {
        m_SmoothIndices[3 * i + 0] = m_FaceIndices[i][0];
        m_SmoothIndices[3 * i + 1] = m_FaceIndices[i][1];
        m_SmoothIndices[3 * i + 2] = m_FaceIndices[i][2];
    }
}

void Mesh::Destroy()
{
    m_VertexPos.clear();
    m_FaceIndices.clear();
    m_FaceNormals.clear();

    delete[] m_FlatVertices;
    delete[] m_FlatIndices;

    m_SmoothVertexNormals.clear();
    delete[] m_SmoothVertices;
    delete[] m_SmoothIndices;
}

void Mesh::Reload(const char* filename)
{
    Destroy();
    loadOBJ(filename);
}

void Mesh::preRender(int shading)
{
    if (shading == FLAT)
    {
        m_OutNumVert = m_FlatNumVert;
        m_OutVertices = m_FlatVertices;
        m_OutNumIdx = m_FlatNumIdx;
        m_OutIndices = m_FlatIndices;
    }
    else if (shading == SMOOTH)
    {
        m_OutNumVert = m_SmoothNumVert;
        m_OutVertices = m_SmoothVertices;
        m_OutNumIdx = m_SmoothNumIdx;
        m_OutIndices = m_SmoothIndices;
    }
}
