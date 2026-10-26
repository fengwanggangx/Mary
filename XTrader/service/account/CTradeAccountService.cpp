#include "CTradeAccountService.h"
#include "../../system/CSession.h"
#include <algorithm>
CTradeAccountService::CTradeAccountService() = default;
CTradeAccountService::~CTradeAccountService()
{
	m_timeouts.request_stop();
	m_wake.notify_all();
	if (m_timeouts.joinable())
	{
		m_timeouts.join();
	}
}
void CTradeAccountService::Initialize()
{
	std::call_once(m_initializeFlag, [this]()
	{
		CSession::InstanceRef().RegisterResponseHandler(
			[this](const CRequest& response)
		{
			OnResponse(response);
		});
		CSession::InstanceRef().RegisterStateHandler(
			[this](SessionState, const std::string&)
		{
			CResult result;
			result.m_bSessionReset = true;
			{
				std::lock_guard<std::mutex> lock(m_mtx_state);
				m_pending.clear();
				m_accounts.clear();
				m_supported = true;
				result.m_generation = ++m_generation;
			}
			m_results.Notify(result);
		});
		m_timeouts = std::jthread([this](std::stop_token stop)
		{
			while (!stop.stop_requested())
			{
				std::vector<CRequest> expired;
				{
					std::unique_lock<std::mutex> lock(m_mtx_state);
					m_wake.wait_for(lock, std::chrono::seconds(1),
						[&stop]()
					{
						return stop.stop_requested();
					});
					if (stop.stop_requested())
					{
						return;
					}
					for (const auto& item : m_pending)
					{
						if (item.second.m_deadline <= std::chrono::steady_clock::now())
						{
							CRequest response;
							response.SetId(item.first);
							response.SetType(item.second.m_type);
							response.SetErrorInfo(1203, "账户请求超时");
							expired.emplace_back(std::move(response));
						}
					}
				}
				for (const auto& response : expired)
				{
					OnResponse(response);
				}
			}
		});
	});
}
_TyCallbackId CTradeAccountService::Subscribe(_TyHandler&& handler)
{
	Initialize();
	return m_results.Subscribe(std::move(handler));
}
void CTradeAccountService::Unsubscribe(_TyCallbackId token)
{
	m_results.Unsubscribe(token);
}
std::vector<request::TradeAccountRecord>
CTradeAccountService::GetAccounts() const
{
	std::lock_guard<std::mutex> lock(m_mtx_state);
	return m_accounts;
}
bool CTradeAccountService::IsSupported() const
{
	std::lock_guard<std::mutex> lock(m_mtx_state);
	return m_supported;
}
std::uint64_t CTradeAccountService::GetGeneration() const
{
	std::lock_guard<std::mutex> lock(m_mtx_state);
	return m_generation;
}
std::uint64_t CTradeAccountService::Send(
	_TyRequest type, std::uint64_t accountId,
	const std::unordered_map<std::string, std::string>& extra)
{
	Initialize();
	if (!CSession::InstanceRef().IsAuthenticated())
	{
		return 0;
	}
	if ((_TyRequest::TRADE_ACCOUNT_QUERY == type) &&
		((0 != accountId) || !extra.empty()))
	{
		return 0;
	}
	CRequest req;
	req.SetType(type);
	switch (type)
	{
	case _TyRequest::TRADE_ACCOUNT_LOGIN:
	case _TyRequest::TRADE_ACCOUNT_QUERY:
	case _TyRequest::TRADE_ACCOUNT_ADD:
	case _TyRequest::TRADE_ACCOUNT_MODIFY:
	case _TyRequest::TRADE_ACCOUNT_CONNECT:
	case _TyRequest::TRADE_ACCOUNT_DISCONNECT:
	case _TyRequest::TRADE_ACCOUNT_INFO_QUERY:
	case _TyRequest::TRADE_ACCOUNT_LOGS_QUERY:
		break;
	default:
		return 0;
	}
	if (_TyRequest::TRADE_ACCOUNT_INFO_QUERY == type &&
		(0 == accountId || extra.contains("account_ids")))
	{
		return 0;
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
			if ((type == item.second.m_type) &&
				(accountId == item.second.m_accountId))
			{
				return 0;
			}
		}
		m_pending.emplace(req.GetId(), CPending{ accountId, type,
										   std::chrono::steady_clock::now() +
											   std::chrono::seconds(10) });
	}
	if (CSession::InstanceRef().SendRequest(req))
	{
		return req.GetId();
	}
	std::lock_guard<std::mutex> lock(m_mtx_state);
	m_pending.erase(req.GetId());
	return 0;
}
void CTradeAccountService::OnResponse(const CRequest& response)
{
	CResult result;
	{
		std::lock_guard<std::mutex> lock(m_mtx_state);
		const auto mIter = m_pending.find(response.GetId());
		if (m_pending.end() == mIter)
		{
			return;
		}
		if (response.GetType() != mIter->second.m_type)
		{
			return;
		}
		result.m_accountId = mIter->second.m_accountId;
		result.m_type = mIter->second.m_type;
		result.m_requestId = response.GetId();
		result.m_generation = m_generation;
		m_pending.erase(mIter);
		result.m_data.CopyFrom(response.GetData());
		std::optional<std::pair<int, std::string>> error = response.GetErrorInfo();
		if (error.has_value())
		{
			result.m_error = error->second.empty() ? "账户请求失败，错误码：" +
														 std::to_string(error->first)
												   : error->second;
			if ((_TyRequest::TRADE_ACCOUNT_QUERY == result.m_type) &&
				((std::string::npos != result.m_error.find("unsupported")) ||
					(std::string::npos != result.m_error.find("unknown"))))
			{
				m_supported = false;
				m_accounts.clear();
			}
		}
		else if (_TyRequest::TRADE_ACCOUNT_INFO_QUERY == result.m_type)
		{
			if (!result.m_data.has_counter_reply() ||
				!result.m_data.counter_reply().has_funds() ||
				result.m_accountId != result.m_data.counter_reply().account_id() ||
				result.m_accountId !=
					result.m_data.counter_reply().funds().account_id())
			{
				result.m_error = "账户响应缺少指定账户信息";
			}
			else if (0 != result.m_data.counter_reply().error_code())
			{
				result.m_error = result.m_data.counter_reply().error_message();
			}
		}
		else if (_TyRequest::TRADE_ACCOUNT_LOGS_QUERY == result.m_type &&
				 !result.m_data.has_account_logs())
		{
			result.m_error = "账户响应缺少有效数据";
		}
		else if ((_TyRequest::TRADE_ACCOUNT_LOGIN == result.m_type) ||
				 (_TyRequest::TRADE_ACCOUNT_ADD == result.m_type) ||
				 (_TyRequest::TRADE_ACCOUNT_MODIFY == result.m_type) ||
				 (_TyRequest::TRADE_ACCOUNT_CONNECT == result.m_type) ||
				 (_TyRequest::TRADE_ACCOUNT_DISCONNECT == result.m_type))
		{
			std::erase_if(m_pending, [](const auto& item)
			{
				return (_TyRequest::TRADE_ACCOUNT_QUERY == item.second.m_type) ||
					   (_TyRequest::TRADE_ACCOUNT_INFO_QUERY == item.second.m_type) ||
					   (_TyRequest::TRADE_ACCOUNT_LOGS_QUERY == item.second.m_type);
			});
		}
		else if (_TyRequest::TRADE_ACCOUNT_QUERY == result.m_type)
		{
			if (!result.m_data.has_account_list())
			{
				result.m_error = "当前 XEngine 不支持账户管理";
				m_supported = false;
				m_accounts.clear();
			}
			else
			{
				m_accounts.assign(result.m_data.account_list().accounts().begin(),
					result.m_data.account_list().accounts().end());
				std::sort(m_accounts.begin(), m_accounts.end(),
					[](const request::TradeAccountRecord& left,
						const request::TradeAccountRecord& right)
				{
					return left.account_id() < right.account_id();
				});
			}
		}
	}
	m_results.Notify(result);
}
