#include "Objects.h"

Objects::Objects()
{
}

Objects::~Objects()
{
	m_Objects.clear();
}

Object Objects::findObj(unsigned int obj)
{
	auto search = m_Objects.find(obj);
	if (search != m_Objects.end())
	{
		return search->second;
	}
	// if search fails, insert it
	Object newObj(m_Filepaths[obj]);
	m_Objects.insert({ obj, newObj });
	return newObj;
}
