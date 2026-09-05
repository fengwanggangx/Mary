#include "CServerSettings.h"
#include "../ini/CINIHandler.h"
#include <array>
#include <charconv>
#include <stdexcept>
#include <string_view>

#include <QDir>
#include <QSettings>

#include <algorithm>
#include <utility>

namespace configuration
{
	namespace
	{
		bool SameHostKey(const std::string &left, const std::string &right)
		{
			return 0 == QString::compare(QString::fromStdString(left), QString::fromStdString(right), Qt::CaseInsensitive);
		}

		void CheckHost(const CHostInfo &host)
		{
			if (!host.Valid() || host.m_strKey.empty() ||
				std::string::npos != host.m_strKey.find_first_of("=;#[] \t\r\n") ||
				std::string::npos != host.m_strKey.find('\0'))
			{
				throw std::invalid_argument("Invalid host or INI key");
			}
		}

		QString MakeSiteKey(const QString &siteId, const QString &field)
		{
			return "Server/" + siteId + "." + field;
		}
	} // namespace

	bool CHostInfo::Valid() const
	{
		return !m_strName.empty() && !m_strHost.empty() && 0 < m_port && 65535 >= m_port &&
			   std::string::npos == m_strName.find_first_of("_\r\n") && std::string::npos == m_strName.find('\0') &&
			   std::string::npos == m_strHost.find_first_of("_\r\n") && std::string::npos == m_strHost.find('\0');
	}

	bool CHostInfo::Deserialize(const std::string &strKey, const std::string &strInfo)
	{
		if (strKey.empty())
		{
			return false;
		}
		std::array<std::string_view, 4> fields;
		std::string_view remaining(strInfo);
		for (std::size_t i = 0; i < 3; ++i)
		{
			std::size_t end = remaining.find('_');
			if (std::string_view::npos == end || 0 == end)
			{
				return false;
			}
			fields[i] = remaining.substr(0, end);
			remaining.remove_prefix(end + 1);
		}
		fields[3] = remaining;
		unsigned int port = 0;
		std::from_chars_result parsed = std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), port);
		if (std::errc{} != parsed.ec || fields[2].data() + fields[2].size() != parsed.ptr ||
			0 == port || 65535 < port || ("0" != fields[3] && "1" != fields[3]))
		{
			return false;
		}
		CHostInfo host;
		host.m_strKey = strKey;
		host.m_strName = fields[0];
		host.m_strHost = fields[1];
		host.m_port = port;
		host.m_bEnabled = "1" == fields[3];
		if (!host.Valid())
		{
			return false;
		}
		*this = std::move(host);
		return true;
	}

	std::string CHostInfo::Serialize() const
	{
		if (!Valid())
		{
			return {};
		}
		return m_strName + "_" + m_strHost + "_" + std::to_string(m_port) + "_" + (m_bEnabled ? "1" : "0");
	}

	void CHostMgr::Initialize()
	{
		auto entries = ini::CINIHandler::InstanceRef().GetSection(ini::Config::System, "Server");
		m_hosts.clear();
		m_hosts.reserve(entries.size());
		for (const auto &entry : entries)
		{
			CHostInfo host;
			if (host.Deserialize(entry.first, entry.second))
			{
				m_hosts.emplace_back(std::move(host));
			}
		}
	}

	std::optional<CHostInfo> CHostMgr::GetActiveHost() const
	{
		for (const auto &host : m_hosts)
		{
			if (host.m_bEnabled && host.Valid())
			{
				return host;
			}
		}
		return std::nullopt;
	}

	void CHostMgr::Add(const CHostInfo &v)
	{
		CheckHost(v);
		for (const auto &host : m_hosts)
		{
			if (SameHostKey(host.m_strKey, v.m_strKey))
			{
				throw std::invalid_argument("Duplicate host key");
			}
		}
		CHostInfo host = v;
		m_hosts.reserve(m_hosts.size() + 1);
		if (!ini::CINIHandler::InstanceRef().UpdateEntry(ini::Config::System, "Server", host.m_strKey, host.Serialize()))
		{
			throw std::runtime_error("Failed to save host");
		}
		m_hosts.emplace_back(std::move(host));
	}

	void CHostMgr::Remove(std::size_t idx)
	{
		const CHostInfo &host = m_hosts.at(idx);
		if (!ini::CINIHandler::InstanceRef().UpdateEntry(ini::Config::System, "Server", host.m_strKey, std::nullopt))
		{
			throw std::runtime_error("Failed to remove host");
		}
		m_hosts.erase(m_hosts.begin() + static_cast<std::vector<CHostInfo>::difference_type>(idx));
	}

	void CHostMgr::Modify(std::size_t idx, const CHostInfo &v)
	{
		const CHostInfo &original = m_hosts.at(idx);
		CheckHost(v);
		for (std::size_t i = 0; i < m_hosts.size(); ++i)
		{
			if (idx != i && SameHostKey(m_hosts[i].m_strKey, v.m_strKey))
			{
				throw std::invalid_argument("Duplicate host key");
			}
		}
		CHostInfo host = v;
		std::string oldKey = SameHostKey(original.m_strKey, host.m_strKey) ? std::string{} : original.m_strKey;
		if (!ini::CINIHandler::InstanceRef().UpdateEntry(ini::Config::System, "Server", host.m_strKey, host.Serialize(), oldKey))
		{
			throw std::runtime_error("Failed to modify host");
		}
		m_hosts[idx] = std::move(host);
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

	bool CServerSettings::SaveSiteIds(const QStringList &siteIds)
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
		for (const QString &siteId : siteIds)
		{
			CServerSite site = LoadSite(siteId);
			if (!site.id.isEmpty())
			{
				sites.emplace_back(std::move(site));
			}
		}
		return sites;
	}

	CServerSite CServerSettings::LoadSite(const QString &siteId)
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

	bool CServerSettings::SetActiveSiteId(const QString &siteId)
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

	bool CServerSettings::SaveSite(const CServerSite &site)
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

	bool CServerSettings::RemoveSite(const QString &siteId)
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
} // namespace configuration
