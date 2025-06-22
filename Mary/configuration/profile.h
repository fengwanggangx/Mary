#ifndef __PROFILE_H__
#define __PROFILE_H__
#include "iniparser.h"
namespace ini
{
	class Profile
	{
	public:
		Profile(const std::string& strFileName);
		~Profile();

	public:
		std::string GetProfileString(const std::string& strSegment, const std::string& strKey, const std::string& strDefault) const;
		int GetProfileInt(const std::string& strSegment, const std::string& strKey, int nDefault) const;
		bool GetProfileBool(const std::string& strSegment, const std::string& strKey, bool bDefault) const;

		void WriteProfileString(const std::string& strSegment, const std::string& strKey, const std::string& strVal);
		void WriteProfileInt(const std::string& strSegment, const std::string& strKey, int nVal);
		void WriteProfileBool(const std::string& strSegment, const std::string& strKey, bool bVal);

		void RemoveProfile(const std::string& strSegment);
		void RemoveProfile(const std::string& strSegment, const std::string& strKey);
	private:
		Profile(const Profile&) = delete;
		Profile(const Profile&&) = delete;
		Profile& operator=(const Profile&) = delete;
		Profile& operator=(const Profile&&) = delete;

	private:
		IniParser m_ini;
		bool	m_bItemChanged{ false };
	};
}

ini::Profile& GetProfile();

#endif
