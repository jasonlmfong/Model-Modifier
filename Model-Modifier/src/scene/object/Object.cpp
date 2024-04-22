#include "Object.h"

Object::Object()
    : m_Min(0), m_Max(0)
{
}

Object::Object(const char* filename)
{
    loadOBJ(filename);
    if (m_NumPolygons.size() == 1 && m_NumPolygons.count(3) == 1)
        m_TriFaceIndices = m_FaceIndices;
    else
        TriangulateFaces();
    Rescale();
}

Object::~Object()
{
    Destroy();
}

void customSplit(std::string str, char separator, std::vector<std::string>& strings) {
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

void Object::loadOBJ(const char* filename)
{
    // initialize the min and max values
    m_Min = glm::vec3{ 1000000, 1000000, 1000000 };
    m_Max = glm::vec3{ -1000000, -1000000, -1000000 };

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

            for (int coord = 0; coord < 3; coord++) // update min and max
            {
                if (v[coord] > m_Max[coord])
                    m_Max[coord] = v[coord];
                if (v[coord] < m_Min[coord])
                    m_Min[coord] = v[coord];
            }
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
            m_NumPolygons[static_cast<int>(faceIndices.size())] += 1;
        }
    }
}

void Object::Rescale()
{
    glm::vec3 lengths = m_Max - m_Min;
    float longest = lengths.x;
    if (lengths.y > longest)
        longest = lengths.y;
    if (lengths.z > longest)
        longest = lengths.z;
    glm::vec3 ratios = lengths / longest;

    for (int i = 0; i < m_VertexPos.size(); i++)
    {
        glm::vec3 vert = m_VertexPos[i];
        glm::vec3 newVert { 0, 0, 0 };
        for (int coord = 0; coord < 3; coord++)
        {
            newVert[coord] = ratios[coord] * ((vert[coord] - m_Min[coord]) * 2 / lengths[coord] - 1);
        }
        m_VertexPos[i] = newVert;
    }
}

void Object::Destroy()
{
    m_VertexPos.clear();
    m_FaceIndices.clear();
}

void Object::Reload(const char* filename)
{
    Destroy();
    loadOBJ(filename);
    Rescale();
}

void Object::TriangulateFaces()
{
    std::vector<std::vector<unsigned int>> triFaces;

    for (size_t i = 0; i < m_FaceIndices.size(); i++)
    {
        if (m_FaceIndices[i].size() == 3)
            triFaces.push_back(m_FaceIndices[i]);
        else if (m_FaceIndices[i].size() > 3)
        {
            int sides = static_cast<int>(m_FaceIndices[i].size());
            std::vector<unsigned int> polygon = m_FaceIndices[i];

            std::vector<std::vector<int>> triangulate = triangulatePolygonalFace(polygon, m_VertexPos);
            for (std::vector<int> triangle : triangulate)
            {
                triFaces.push_back({polygon[triangle[0]], polygon[triangle[1]] , polygon[triangle[2]] });
            }
        }
    }

    m_TriFaceIndices = triFaces;
}
