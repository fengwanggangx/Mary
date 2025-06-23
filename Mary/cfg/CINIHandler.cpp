#include "CINIHandler.h"
#include <fstream>


namespace ini
{
	CINIHandler::CINIHandler()
	{
		Load();
	}

	CINIHandler::~CINIHandler()
	{
	}

	std::unordered_map<ini::cfgs, std::string> s_files;
	bool CINIHandler::Load()
	{
		if (!s_files.empty())
		{
			return false;
		}
		for (const auto& f : s_files)
		{
			m_ini[f.first] = new CIniFile(f.second);
		}
		return true;
	}

	/*
	template <class _Ty>
	_Ty CINIHandler::GetValue(ini::cfgs cfg, const std::string& strSection, const std::string& strKey, _Ty&& def) const
	{
		const auto& mIter = m_ini.find(cfg);
		if ((m_ini.end() == mIter) || (nullptr == mIter->second))
		{
			return def;
		}
		return mIter->second->GetValue(strSection, strKey, def);
	}

	template <class _Ty>
	bool CINIHandler::SetValue(ini::cfgs cfg, const std::string& strSection, const std::string& strKey, _Ty&& val)
	{
		const auto& mIter = m_ini.find(cfg);
		if ((m_ini.end() == mIter) || (nullptr == mIter->second))
		{
			return false;
		}
		return mIter->second->SetValue(strSection, strKey, val);
	}
	*/
}