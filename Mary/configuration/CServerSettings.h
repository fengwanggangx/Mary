#ifndef MARY_CONFIGURATION_CSERVERSETTINGS_H
#define MARY_CONFIGURATION_CSERVERSETTINGS_H

#include "../common/ISingleton.h"

#include <optional>
#include <string>
#include <vector>

namespace configuration
{
	struct CHostInfo
	{
		std::string m_strKey;
		std::string m_strName;
		std::string m_strHost;
		unsigned int m_port{0};
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
		void Initialize();
		std::string CHostMgr::Key() const;
		const std::unordered_map<std::string, CHostInfo>& GetHosts() const;
		std::optional<CHostInfo> GetActiveHost() const;
		// Invalid arguments and persistence failures are reported by exceptions.
		bool Add(const CHostInfo &v);
		bool Remove(std::string& strKey);
		bool Modify(const CHostInfo &v);

	  private:
		std::unordered_map<std::string, CHostInfo> m_hosts;
	};
} // namespace configuration

#endif
