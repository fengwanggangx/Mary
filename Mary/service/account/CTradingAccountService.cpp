#include "CTradingAccountService.h"
#include "../../system/CSession.h"
#include <algorithm>
CTradingAccountService::CTradingAccountService() = default;
CTradingAccountService::~CTradingAccountService() = default;
void CTradingAccountService::Initialize()
{
	std::call_once(m_initializeFlag, [this]()
	{
		CSession::InstanceRef().RegisterResponseHandler([this](const CRequest& response)
		{
			OnResponse(response);
		});
		CSession::InstanceRef().RegisterStateHandler([this](SessionState, const std::string&)
		{
			CResult result;
			result.m_command = "account_session_reset";
			{
				std::lock_guard<std::mutex> lock(m_mtx_state);
				m_pending.clear();
				m_accounts.clear();
				m_supported = true;
				result.m_generation = ++m_generation;
			}
			m_results.Notify(result);
		});
	});
}
_TyCallbackId CTradingAccountService::Subscribe(_TyHandler&& handler)
{
	Initialize();
	return m_results.Subscribe(std::move(handler));
}
void CTradingAccountService::Unsubscribe(_TyCallbackId token)
{
	m_results.Unsubscribe(token);
}
std::vector<request::TradeAccountRecord> CTradingAccountService::GetAccounts() const
{
	std::lock_guard<std::mutex> lock(m_mtx_state);
	return m_accounts;
}
bool CTradingAccountService::IsSupported() const
{
	std::lock_guard<std::mutex> lock(m_mtx_state);
	return m_supported;
}
std::uint64_t CTradingAccountService::GetGeneration() const
{
	std::lock_guard<std::mutex> lock(m_mtx_state);
	return m_generation;
}
std::uint64_t CTradingAccountService::Send(const std::string& command, std::uint64_t accountId, const std::unordered_map<std::string, std::string>& extra)
{
	Initialize();
	if (!CSession::InstanceRef().IsAuthenticated())
	{
		return 0;
	}
	if (("account_query" == command) && ((0 != accountId) || !extra.empty()))
	{
		return 0;
	}
	CRequest req;
	if ("account_query" == command)
	{
		req.SetType(CRequest::Type::TRADE_ACCOUNT_QUERY);
	}
	else
	{
		req.SetType("trade_account_login" == command ? CRequest::Type::TRADE_ACCOUNT_LOGIN : CRequest::Type::ACCOUNT);
		req.SetCmd(command);
	}
	if (0 != accountId)
	{
		req.SetExtraData("account_id", std::to_string(accountId));
	}
	for (const auto& item : extra)
	{
		req.SetExtraData(item.first, item.second);
	}
	{
		std::lock_guard<std::mutex> lock(m_mtx_state);
		if (!m_supported)
		{
			return 0;
		}
		for (const auto& item : m_pending)
		{
			if ((command == item.second.m_command) && (accountId == item.second.m_accountId))
			{
				return 0;
			}
		}
		m_pending.emplace(req.GetId(), CPending{ accountId, command });
	}
	if (CSession::InstanceRef().SendRequest(req))
	{
		return req.GetId();
	}
	std::lock_guard<std::mutex> lock(m_mtx_state);
	m_pending.erase(req.GetId());
	return 0;
}
void CTradingAccountService::OnResponse(const CRequest& response)
{
	CResult result;
	{
		std::lock_guard<std::mutex> lock(m_mtx_state);
		const auto mIter = m_pending.find(response.GetId());
		if (m_pending.end() == mIter)
		{
			return;
		}
		result.m_accountId = mIter->second.m_accountId;
		result.m_command = mIter->second.m_command;
		result.m_requestId = response.GetId();
		result.m_generation = m_generation;
		m_pending.erase(mIter);
		result.m_data.CopyFrom(response.GetData());
		std::optional<std::pair<int, std::string>> error = response.GetErrorInfo();
		if (error.has_value())
		{
			result.m_error = error->second.empty() ? "账户请求失败，错误码：" + std::to_string(error->first) : error->second;
			if (("account_query" == result.m_command) && ((std::string::npos != result.m_error.find("unsupported")) || (std::string::npos != result.m_error.find("unknown"))))
			{
				m_supported = false;
				m_accounts.clear();
			}
		}
		else if ((("account_snapshot_query" == result.m_command) && !result.m_data.has_account_snapshot()) || (("account_logs_query" == result.m_command) && !result.m_data.has_account_logs()))
		{
			result.m_error = "账户响应缺少有效数据";
		}
		else if (("trade_account_login" == result.m_command) || ("account_add" == result.m_command) || ("account_modify" == result.m_command) || ("account_connect" == result.m_command) || ("account_disconnect" == result.m_command))
		{
			std::erase_if(m_pending, [](const auto& item)
			{
				return ("account_query" == item.second.m_command) || ("account_snapshot_query" == item.second.m_command) || ("account_logs_query" == item.second.m_command);
			});
		}
		else if ("account_query" == result.m_command)
		{
			if (!result.m_data.has_account_list())
			{
				result.m_error = "当前 Wind 不支持账户管理";
				m_supported = false;
				m_accounts.clear();
			}
			else
			{
				m_accounts.assign(result.m_data.account_list().accounts().begin(), result.m_data.account_list().accounts().end());
				std::sort(m_accounts.begin(), m_accounts.end(), [](const request::TradeAccountRecord& left, const request::TradeAccountRecord& right)
				{
					return left.account_id() < right.account_id();
				});
			}
		}
	}
	m_results.Notify(result);
}
