#ifndef __CINIHANDLER_H__
#define __CINIHANDLER_H__

#include "./common/ISingleton.h"

#include <string>
#include <unordered_map>


namespace ini
{
	enum class cfgs
	{
		system = 0,
		ui,
		sqlite,
		mysql,
		oracle
	};

	class CIniFile;
	class CINIHandler : public ISingleton<CINIHandler>
	{
		DECLARE_SINGLE_DFAULT(CINIHandler)

	public:
		template <class _Ty>
		_Ty GetValue(ini::cfgs cfg, const std::string& strSection, const std::string& strKey, _Ty&& def) const;

		template <class _Ty>
		bool SetValue(ini::cfgs cfg, const std::string& strSection, const std::string& strKey, _Ty&& val) const;
	
		bool Load();
	private:
		std::unordered_map<ini::cfgs, CIniFile*> m_ini;
	};
}

#endif
