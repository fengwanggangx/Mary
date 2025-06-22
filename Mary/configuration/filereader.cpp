#include "filereader.h"
#include <fstream>
namespace ini
{
	FileReader::FileReader(const std::string& strFileName) : m_strFileName(strFileName)
	{
	}

	FileReader::~FileReader()
	{
	}

	int FileReader::Read()
	{
		return Read(m_strFileName, m_vecText);
	}

	int FileReader::Read(const std::string& strFileName, std::vector<std::string>& vecText)
	{
		if (strFileName.empty())
		{
			return -1;
		}
		if (vecText.capacity() < 1000)
		{
			vecText.reserve(1000);
		}

		std::ifstream file(strFileName.c_str(), std::ifstream::in);
		if (!file.is_open())
		{
			return -1;
		}
		std::string strText;
		while (file.good())
		{
			file >> strText;
			vecText.emplace_back(strText);
		}
		file.close();
		return vecText.size();
	}

	std::string FileReader::GetFileName() const
	{
		return m_strFileName;
	}

	const std::vector<std::string>& FileReader::GetText() const
	{
		return m_vecText;
	}
}