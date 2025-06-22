#include "iniparser.h"
#include <regex>

namespace ini
{
	IniParser::IniParser(const std::string& strFileName) : m_reader(strFileName), m_writer(strFileName)
	{
		m_reader.Read();
		std::vector<std::string> vecFile = m_reader.GetText();
		Parse(m_mapIniText, vecFile);
	}

	IniParser::~IniParser()
	{
		std::vector<std::string>& vecFile = m_writer.GetText();
		Parse(vecFile, m_mapIniText);
		m_writer.Write();
	}

	const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& IniParser::GetIniText() const
	{
		return m_mapIniText;
	}

	std::unordered_map<std::string, std::string>& IniParser::GetSegmentText(const std::string& strSegment)
	{
		return m_mapIniText[strSegment];
	}

	std::string IniParser::GetItemText(const std::string& strSegment, const std::string& strKey) const
	{
		auto mIter = m_mapIniText.find(strSegment);
		if (mIter != m_mapIniText.end())
		{
			auto mmIter = mIter->second.find(strKey);
			if ((mmIter != mIter->second.end()) && (!mmIter->second.empty()))
			{
				return mmIter->second;
			}
		}
		return "";
	}

	void IniParser::SetItemText(const std::string& strSegment, const std::string& strKey, const std::string& strValue)
	{
		m_mapIniText[strSegment][strKey] = strValue;//有则改，无则造
	}

	void IniParser::RemoveText(const std::string& strSegment)
	{
		m_mapIniText.erase(strSegment);
	}

	void IniParser::RemoveText(const std::string& strSegment, const std::string& strKey)
	{
		auto mIter = m_mapIniText.find(strSegment);
		if (mIter != m_mapIniText.end())
		{
			mIter->second.erase(strKey);
		}
	}

	//. 一个字符，写几个是几个
	//* 前面字符一个或多个
	//$  以结尾
	//^  以开头
	//()匹配其中的固定串 如(abc)匹配abc, 里面可以使用|标市如(abc|cde) 表示abc或者cde其中一个
	// + 前面字符1到多个
	int IniParser::Parse(std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& mapIniText, const std::vector<std::string>& vecText)
	{
		mapIniText.clear();
		if (vecText.size() <= 0)
		{
			return 0;
		}
		//std::regex ptnSection("^\\[.+\\]$");
		std::regex ptnSection("(\\[)([a-zA-Z0-9]+)(\\])");
		std::regex ptnValue("([a-zA-Z0-9]+)=(.+)");
		std::smatch res;
		std::string strSection;
		for (auto& item : vecText)
		{
			if (std::regex_match(item, res, ptnSection))
			{
				strSection = res[2];
				mapIniText[strSection];
			}
			else if (std::regex_match(item, res, ptnValue))
			{
				std::string strKey = res[1];
				std::string strValue = res[2];
				mapIniText[strSection].emplace(strKey, strValue);
			}
		}
		return mapIniText.size();
	}

	int IniParser::Parse(std::vector<std::string>& vecText, const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& mapIniText)
	{
		vecText.clear();
		for (auto &item : mapIniText)
		{
			vecText.emplace_back('[' + item.first + ']');
			for (auto &iitem : item.second)
			{
				vecText.emplace_back(iitem.first + '=' + iitem.second);
			}
		}
		return vecText.size();
	}
}