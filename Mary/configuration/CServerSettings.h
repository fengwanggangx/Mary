#ifndef MARY_CONFIGURATION_CSERVERSETTINGS_H
#define MARY_CONFIGURATION_CSERVERSETTINGS_H

#include "../common/ISingleton.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace configuration
{
	struct CHostInfo
	{
		std::string m_strKey;
		std::string m_strName;
		std::string m_strHost;
		unsigned int m_nPort{ 0 };

		bool m_bEnabled{ false };

		bool operator==(const CHostInfo& arg) const;

		bool Valid() const;

		std::string Serialize() const;
		bool Deserialize(const std::string &strKey, const std::string &strInfo);
	};

	class CHostMgr final : public ISingleton<CHostMgr>
	{
		DECLARE_SINGLE_DFAULT(CHostMgr)

	  public:
		std::string Key() const;
		const std::unordered_map<std::string, CHostInfo>& GetHosts() const;
		std::optional<CHostInfo> GetActiveHost() const;
		// Invalid arguments and persistence failures are reported by exceptions.
		bool Add(const CHostInfo &v);
		bool Remove(const std::string& strKey);
		bool Modify(const CHostInfo &v);

		bool SetConnectFast(bool v);
		bool IsConnectFast() const;

	private:
		void Initialize();

	  private:
		bool m_bConnectFast{ false };
		std::unordered_map<std::string, CHostInfo> m_hosts;
	};
} // namespace configuration

#endif
