#include "Objects.h"

Objects::Objects()
{
}

Objects::~Objects()
{
	m_Objects.clear();
}

Object Objects::findObj(int obj)
{
	if (m_Objects.find(obj) == m_Objects.end())
	{
		Object newObj(m_Filepaths[obj]);
		m_Objects.insert({ obj, newObj });
	}
	return m_Objects.find(obj)->second;
}
