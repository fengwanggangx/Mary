#include "CServerSettings.h"
#include "../ini/CINIHandler.h"
#include "../common/utility.h"
#include <array>
#include <charconv>
#include <stdexcept>
#include <string_view>

#include <algorithm>
#include <utility>

namespace configuration
{
	static constexpr unsigned int cs_port_low_limit = 0;
	static constexpr unsigned int cs_port_up_limit = 65535;

	CHostMgr::CHostMgr()
	{
		Initialize();
	}

	CHostMgr::~CHostMgr() = default;

	bool CHostInfo::Valid() const
	{
		return !m_strName.empty() && !m_strHost.empty() && utility::between(m_nPort, cs_port_low_limit, cs_port_up_limit);
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

		unsigned int nPort = 0;
		if (!utility::to_number(std::string(v.at(2)), nPort) || !utility::between(nPort, cs_port_low_limit, cs_port_up_limit))
		{
			return false;
		}

		m_strKey = strKey;
		m_strName = v.at(0);
		m_strHost = v.at(1);
		m_nPort = nPort;
		m_bEnabled = "1" == v.at(3);
		return true;
	}

	bool CHostInfo::operator==(const CHostInfo& arg) const
	{
		return (m_strHost == arg.m_strHost) && (m_nPort == arg.m_nPort);
	}

	std::string CHostInfo::Serialize() const
	{
		if (!Valid())
		{
			return {};
		}
		return m_strName + "_" + m_strHost + "_" + std::to_string(m_nPort) + "_" + (m_bEnabled ? "1" : "0");
	}

	void CHostMgr::Initialize()
	{
		auto entries = ini::CINIHandler::InstanceRef().GetSection(ini::Config::System, "Server");
		m_hosts.clear();
		m_hosts.reserve(entries.size());
		for (const auto& [k, v] : entries)
		{
			if (k == "connect_fast")
			{
				m_bConnectFast = "1" == v;
			}
			else if (k.find("host") == 0)
			{
				CHostInfo host;
				if (host.Deserialize(k, v))
				{
					m_hosts.emplace(k, std::move(host));
				}
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
		for (const auto& v : m_hosts)
		{
			const auto& info = v.second;
			if (info.m_bEnabled && info.Valid())
			{
				return info;
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

		if (!ini::CINIHandler::InstanceRef().UpdateEntry(ini::Config::System, "Server", v.m_strKey, v.Serialize()))
		{
			return false;
		}
		m_hosts.emplace(v.m_strKey, v);
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
		return true;
	}

	bool CHostMgr::SetConnectFast(bool v)
	{
		bool bRet = m_bConnectFast;
		m_bConnectFast = v;
		return bRet;
	}

	bool CHostMgr::IsConnectFast() const
	{
		return m_bConnectFast;
	}

} // namespace configuration
