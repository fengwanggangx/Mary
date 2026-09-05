#include "CServerSettings.h"
#include "../ini/CINIHandler.h"
#include <array>
#include <charconv>
#include <stdexcept>
#include <string_view>

#include <algorithm>
#include <utility>

namespace configuration
{
	static constexpr unsigned int cs_port_limit = 65535;

	CHostMgr::~CHostMgr() = default;

	bool CHostInfo::Valid() const
	{
		return !m_strName.empty() && !m_strHost.empty() && utility::between(m_port, 0, cs_port_limit);
	}

	bool CHostInfo::Deserialize(const std::string &strKey, const std::string &strInfo)
	{
		if (strKey.empty())
		{
			return false;
		}

		std::vector<std::string_view> v;
		if (4 != utility::split(strInfo, v, '_', false))
		{
			return false;
		}

		int nPort = 0;
		if (!utility::to_number(v.at(2), nPort) || utility::between(nPort, 0, cs_port_limit))
		{
			return false;
		}

		m_strKey = strKey;
		m_strName = v.at(0);
		m_strHost = v.at(1);
		m_port = nPort;
		m_bEnabled = "1" == v.at(3);
		return true;
	}

	bool CHostInfo::operator==(const CHostInfo& arg) const
	{
		return (m_strHost == arg.m_strHost) && (m_port == arg.m_port);
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
				m_hosts.emplace_back(entry.first, std::move(host));
			}
		}
	}

	std::string CHostMgr::Key() const
	{
		return "host" + std::to_string(m_hosts.size());
	}

	const std::unordered_map<std::string, CHostInfo>& CHostMgr::GetHosts() const
	{
		return m_hosts;
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

	bool CHostMgr::Add(const CHostInfo &v)
	{
		if (!v.Valid())
		{
			return false;
		}
		for (const auto& item : m_hosts)
		{
			if (item.second == v)
			{
				return false;
			}
		}

		if (!ini::CINIHandler::InstanceRef().UpdateEntry(ini::Config::System, "Server", m_hosts.m_strKey, v.Serialize()))
		{
			return false;
		}
		m_hosts.emplace_back(m_hosts.m_strKey, v);
		return true;
	}

	bool CHostMgr::Remove(const std::string& strKey)
	{
		const auto mIter = m_hosts.find(strKey);
		if (m_hosts.end() == mIter)
		{
			return false;
		}

		if (!ini::CINIHandler::InstanceRef().UpdateEntry(ini::Config::System, "Server", strKey, std::nullopt))
		{
			return false;
		}
		m_hosts.erase(mIter);
		return true;
	}

	bool CHostMgr::Modify(const CHostInfo &v)
	{
		if (!v.Valid())
		{
			return false;
		}

		if (!ini::CINIHandler::InstanceRef().UpdateEntry(ini::Config::System, "Server", v.m_strKey, v.Serialize()))
		{
			return false;
		}
		m_hosts[v.m_strKey] = v;
	}

} // namespace configuration
