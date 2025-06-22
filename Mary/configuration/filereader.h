#ifndef __FILEREADER_H__
#define __FILEREADER_H__
#include <iostream>
#include <vector>
namespace ini
{
	class FileReader
	{
	public:
		FileReader(const std::string& strFileName);
		~FileReader();

	public:	
		int Read();
		std::string GetFileName() const;
		const std::vector<std::string>& GetText() const;

	private:
		int Read(const std::string& strFileName, std::vector<std::string>& vecText);

	private:
		FileReader(const FileReader&) = delete;
		FileReader(const FileReader&&) = delete;
		FileReader& operator=(const FileReader&) = delete;
		FileReader& operator=(const FileReader&&) = delete;

	private:
		std::vector<std::string>	m_vecText;
		std::string	m_strFileName;
	};
}

#endif
