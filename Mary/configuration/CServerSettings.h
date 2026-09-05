#ifndef MARY_CONFIGURATION_CSERVERSETTINGS_H
#define MARY_CONFIGURATION_CSERVERSETTINGS_H

#include <QString>
#include <QStringList>
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
		bool m_bEnabled{false};

		bool Valid() const;
		bool Deserialize(const std::string &strKey, const std::string &strInfo);
		std::string Serialize() const;
	};

	class CHostMgr final
	{
	  public:
		void Initialize();
		std::optional<CHostInfo> GetActiveHost() const;
		// Invalid arguments and persistence failures are reported by exceptions.
		void Add(const CHostInfo &v);
		void Remove(std::size_t idx);
		void Modify(std::size_t idx, const CHostInfo &v);

	  private:
		std::vector<CHostInfo> m_hosts;
	};

	struct CServerSite
	{
		QString id;
		QString name;
		QString windHost;
		int windPort{0};
		QString hqMarketHost;
		int hqMarketPort{0};
		bool enabled{true};
	};

	class CServerSettings final
	{
	  public:
		static std::vector<CServerSite> LoadSites();
		static CServerSite LoadSite(const QString &siteId);
		static QString GetActiveSiteId();
		static bool SetActiveSiteId(const QString &siteId);
		static bool SaveSite(const CServerSite &site);
		static bool RemoveSite(const QString &siteId);
		static QString GetFilePath();

	  private:
		static QStringList LoadSiteIds();
		static bool SaveSiteIds(const QStringList &siteIds);
	};
} // namespace configuration

#endif
