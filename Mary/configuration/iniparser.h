#ifndef __INIPARSER_H__
#define __INIPARSER_H__
#include <iostream>
#include <unordered_map>
#include "filereader.h"
#include "filewriter.h"
namespace ini
{
	class IniParser
	{
	public:
		IniParser(const std::string& strFileName);
		~IniParser();

	public:
		const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& GetIniText() const;
		std::unordered_map<std::string, std::string>& GetSegmentText(const std::string& strSegment);
		std::string GetItemText(const std::string& strSegment, const std::string& strKey) const;
		void SetItemText(const std::string& strSegment, const std::string& strKey, const std::string& strValue);
		void RemoveText(const std::string& strSegment);
		void RemoveText(const std::string& strSegment, const std::string& strKey);
	private:
		int Parse(std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& mapIniText, const std::vector<std::string>& vecText);
		int Parse(std::vector<std::string>& vecText, const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& mapIniText);
	private:
		IniParser(const IniParser&) = delete;
		IniParser(const IniParser&&) = delete;
		IniParser& operator=(const IniParser&) = delete;
		IniParser& operator=(const IniParser&&) = delete;

	private:
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>>	m_mapIniText;
		FileReader m_reader;
		FileWriter m_writer;
	};
}

#endif
