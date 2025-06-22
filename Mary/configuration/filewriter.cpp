#include "FileWriter.h"
#include <fstream>
namespace ini
{
	FileWriter::FileWriter(const std::string& strFileName) : m_strFileName(strFileName)
	{
	}

	FileWriter::~FileWriter()
	{
	}

	int FileWriter::Write()
	{
		return Write(m_strFileName, m_vecText);
	}

	int FileWriter::Write(const std::string& strFileName, std::vector<std::string>& vecText)
	{
		if (strFileName.empty())
		{
			return -1;
		}

		std::ofstream file(strFileName.c_str(), std::ofstream::out | std::ofstream::binary);
		if (!file.is_open())
		{
			return -1;
		}
		std::string strText;
		size_t nCount = vecText.size();
		for (size_t i = 0; i < nCount; i++)
		{
			if (file.good())
			{
				if (i >= (nCount - 1))
				{
					file << vecText[i];
				}
				else
				{
					file << vecText[i] << std::endl;
				}
			}
		}
		file.close();
		return vecText.size();
	}

	std::string FileWriter::GetFileName() const
	{
		return m_strFileName;
	}

	std::vector<std::string>& FileWriter::GetText()
	{
		return m_vecText;
	}

	const std::vector<std::string>& FileWriter::GetText() const
	{
		return m_vecText;
	}
}