#ifndef MARY_CONFIGURATION_CSERVERSETTINGS_H
#define MARY_CONFIGURATION_CSERVERSETTINGS_H

#include <QString>
#include <QStringList>
#include <vector>

namespace configuration
{
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
		static CServerSite LoadSite(const QString& siteId);
		static QString GetActiveSiteId();
		static bool SetActiveSiteId(const QString& siteId);
		static bool SaveSite(const CServerSite& site);
		static bool RemoveSite(const QString& siteId);
		static QString GetFilePath();

	private:
		static QStringList LoadSiteIds();
		static bool SaveSiteIds(const QStringList& siteIds);
	};
}

#endif
