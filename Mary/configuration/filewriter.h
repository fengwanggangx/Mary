#ifndef __FileWriter_H__
#define __FileWriter_H__
#include <iostream>
#include <vector>
namespace ini
{
	class FileWriter
	{
	public:
		FileWriter(const std::string& strFileName);
		~FileWriter();

	public:
		int Write();
		std::string GetFileName() const;
		std::vector<std::string>& GetText();
		const std::vector<std::string>& GetText() const;
	private:
		int Write(const std::string& strFileName, std::vector<std::string>& vecText);
	private:
		FileWriter(const FileWriter&) = delete;
		FileWriter(const FileWriter&&) = delete;
		FileWriter& operator=(const FileWriter&) = delete;
		FileWriter& operator=(const FileWriter&&) = delete;

	private:
		std::vector<std::string>	m_vecText;
		std::string	m_strFileName;
	};
}

#endif
