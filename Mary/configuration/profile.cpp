#include "profile.h"
#include "../common/singleton.h"
#include <string>

ini::Profile& GetProfile()
{
	std::string strp = "C:/Users/X/Desktop/xxx.ini";
	return  Singleton<ini::Profile>::Instance(strp);
}

namespace ini
{
	Profile::Profile(const std::string& strFileName) : m_ini(strFileName)
	{
	}
	Profile::~Profile()
	{
	}

	std::string Profile::GetProfileString(const std::string& strSegment, const std::string& strKey, const std::string& strDefault) const
	{
		std::string strVal = m_ini.GetItemText(strSegment, strKey);
		return (strVal.empty() ? strDefault : strVal);
	}

	int Profile::GetProfileInt(const std::string& strSegment, const std::string& strKey, int nDefault) const
	{
		std::string strVal = m_ini.GetItemText(strSegment, strKey);
		return (strVal.empty() ? nDefault : atoi(strVal.c_str()));
	}

	bool Profile::GetProfileBool(const std::string& strSegment, const std::string& strKey, bool bDefault) const
	{
		std::string strVal = m_ini.GetItemText(strSegment, strKey);
		return (strVal.empty() ? bDefault : (strVal.compare("true") == 0));
	}

	void Profile::WriteProfileString(const std::string& strSegment, const std::string& strKey, const std::string& strVal)
	{
		m_ini.SetItemText(strSegment, strKey, strVal);
	}

	void Profile::WriteProfileInt(const std::string& strSegment, const std::string& strKey, int nVal)
	{
		WriteProfileString(strSegment, strKey, std::to_string(nVal));
	}

	void Profile::WriteProfileBool(const std::string& strSegment, const std::string& strKey, bool bVal)
	{
		WriteProfileString(strSegment, strKey, bVal ? "true" : "false");
	}

	void Profile::RemoveProfile(const std::string& strSegment)
	{
		m_ini.RemoveText(strSegment);
	}

	void Profile::RemoveProfile(const std::string& strSegment, const std::string& strKey)
	{
		m_ini.RemoveText(strSegment, strKey);
	}
}