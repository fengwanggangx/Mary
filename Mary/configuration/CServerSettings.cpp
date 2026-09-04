#include "CServerSettings.h"

#include <QDir>
#include <QSettings>

#include <algorithm>
#include <utility>

namespace configuration
{
	namespace
	{
		QString MakeSiteKey(const QString& siteId, const QString& field)
		{
			return "Server/" + siteId + "." + field;
		}
	}

	QString CServerSettings::GetFilePath()
	{
		return QDir::current().filePath("ini/system.ini");
	}

	QStringList CServerSettings::LoadSiteIds()
	{
		QSettings settings(GetFilePath(), QSettings::IniFormat);
		return settings.value("Server/sites").toStringList();
	}

	bool CServerSettings::SaveSiteIds(const QStringList& siteIds)
	{
		QSettings settings(GetFilePath(), QSettings::IniFormat);
		settings.setValue("Server/sites", siteIds);
		settings.sync();
		return QSettings::NoError == settings.status();
	}

	std::vector<CServerSite> CServerSettings::LoadSites()
	{
		QStringList siteIds = LoadSiteIds();
		std::vector<CServerSite> sites;
		sites.reserve(static_cast<std::size_t>(siteIds.size()));
		for (const QString& siteId : siteIds)
		{
			CServerSite site = LoadSite(siteId);
			if (!site.id.isEmpty())
			{
				sites.emplace_back(std::move(site));
			}
		}
		return sites;
	}

	CServerSite CServerSettings::LoadSite(const QString& siteId)
	{
		CServerSite site;
		if (siteId.isEmpty())
		{
			return site;
		}

		QSettings settings(GetFilePath(), QSettings::IniFormat);
		if (!LoadSiteIds().contains(siteId))
		{
			return site;
		}

		site.id = siteId;
		site.name = settings.value(MakeSiteKey(siteId, "name")).toString();
		site.windHost = settings.value(MakeSiteKey(siteId, "wind.host")).toString();
		site.windPort = settings.value(MakeSiteKey(siteId, "wind.port")).toInt();
		site.hqMarketHost = settings.value(MakeSiteKey(siteId, "hqmarket.host")).toString();
		site.hqMarketPort = settings.value(MakeSiteKey(siteId, "hqmarket.port")).toInt();
		site.enabled = settings.value(MakeSiteKey(siteId, "enabled"), true).toBool();
		return site;
	}

	QString CServerSettings::GetActiveSiteId()
	{
		QSettings settings(GetFilePath(), QSettings::IniFormat);
		return settings.value("Server/active").toString();
	}

	bool CServerSettings::SetActiveSiteId(const QString& siteId)
	{
		if (!LoadSiteIds().contains(siteId))
		{
			return false;
		}
		QSettings settings(GetFilePath(), QSettings::IniFormat);
		settings.setValue("Server/active", siteId);
		settings.sync();
		return QSettings::NoError == settings.status();
	}

	bool CServerSettings::SaveSite(const CServerSite& site)
	{
		if (site.id.isEmpty() || site.name.isEmpty())
		{
			return false;
		}

		QStringList siteIds = LoadSiteIds();
		if (!siteIds.contains(site.id))
		{
			siteIds.append(site.id);
		}
		QSettings settings(GetFilePath(), QSettings::IniFormat);
		settings.setValue("Server/sites", siteIds);
		settings.setValue(MakeSiteKey(site.id, "name"), site.name);
		settings.setValue(MakeSiteKey(site.id, "wind.host"), site.windHost);
		settings.setValue(MakeSiteKey(site.id, "wind.port"), site.windPort);
		settings.setValue(MakeSiteKey(site.id, "hqmarket.host"), site.hqMarketHost);
		settings.setValue(MakeSiteKey(site.id, "hqmarket.port"), site.hqMarketPort);
		settings.setValue(MakeSiteKey(site.id, "enabled"), site.enabled);
		settings.sync();
		return QSettings::NoError == settings.status();
	}

	bool CServerSettings::RemoveSite(const QString& siteId)
	{
		QStringList siteIds = LoadSiteIds();
		if (!siteIds.removeOne(siteId))
		{
			return false;
		}
		QSettings settings(GetFilePath(), QSettings::IniFormat);
		settings.remove("Server/" + siteId + ".name");
		settings.remove("Server/" + siteId + ".wind.host");
		settings.remove("Server/" + siteId + ".wind.port");
		settings.remove("Server/" + siteId + ".hqmarket.host");
		settings.remove("Server/" + siteId + ".hqmarket.port");
		settings.remove("Server/" + siteId + ".enabled");
		settings.setValue("Server/sites", siteIds);
		if (siteId == settings.value("Server/active").toString())
		{
			settings.setValue("Server/active", siteIds.isEmpty() ? QString() : siteIds.front());
		}
		settings.sync();
		return QSettings::NoError == settings.status();
	}
}
