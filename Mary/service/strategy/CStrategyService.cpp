#include "CStrategyService.h"

#include "../../system/CSession.h"

#include <utility>
#include <algorithm>
#include <cctype>

CStrategyService::CStrategyService() = default;

CStrategyService::~CStrategyService() = default;

void CStrategyService::Initialize()
{
	std::call_once(m_initializeFlag, [this]()
	{
		CSession::InstanceRef().RegisterResponseHandler([this](const CRequest& response)
		{
			OnRequestReply(response);
		});
		CSession::InstanceRef().RegisterStateHandler([this](SessionState state, const std::string&)
		{
			if (SessionState::Ready == state)
			{
				QueryStrategies();
			}
		});
	});
}

_TyCallbackId CStrategyService::AddQueryHandler(_TyQueryHandler&& handler)
{
	return m_queryPump.Subscribe([handler = std::move(handler)](const std::pair<_TyStrategyList, std::string>& result)
	{
		handler(result.first, result.second);
	});
}

void CStrategyService::RemoveQueryHandler(_TyCallbackId nToken)
{
	m_queryPump.Unsubscribe(nToken);
}

void CStrategyService::SetQueryHandler(_TyQueryHandler&& handler)
{
	std::lock_guard<std::mutex> lock(m_mtx_handlers);
	m_queryHandler = std::move(handler);
}

void CStrategyService::SetOperationHandler(_TyOperationHandler&& handler)
{
	std::lock_guard<std::mutex> lock(m_mtx_handlers);
	m_operationHandler = std::move(handler);
}

bool CStrategyService::QueryStrategies()
{
	Initialize();
	return CSession::InstanceRef().SendRequest(request::QueryStrategies());
}

bool CStrategyService::AddStrategy(const request::StrategyInfo& strategy)
{
	std::string error;
	if (!ValidateStrategy(strategy, error))
	{
		return false;
	}
	Initialize();
	return CSession::InstanceRef().SendRequest(request::AddStrategy(strategy));
}

bool CStrategyService::ModifyStrategy(const request::StrategyInfo& strategy)
{
	std::string error;
	if ((0 == strategy.strategy_id()) || !ValidateStrategy(strategy, error))
	{
		return false;
	}
	Initialize();
	return CSession::InstanceRef().SendRequest(request::ModifyStrategy(strategy));
}

bool CStrategyService::DeleteStrategy(std::uint64_t id)
{
	if (0 == id)
	{
		return false;
	}
	Initialize();
	return CSession::InstanceRef().SendRequest(request::DeleteStrategy(id));
}

CStrategyService::_TyStrategyList CStrategyService::GetStrategies() const
{
	std::shared_lock<std::shared_mutex> lock(m_mtx_strategies);
	return m_strategies;
}

bool CStrategyService::FindStrategy(std::uint64_t id, request::StrategyInfo& result) const
{
	std::shared_lock<std::shared_mutex> lock(m_mtx_strategies);
	for (const auto& strategy : m_strategies)
	{
		if (id == strategy.strategy_id())
		{
			result = strategy;
			return true;
		}
	}
	return false;
}

bool CStrategyService::IsCacheValid() const
{
	std::shared_lock<std::shared_mutex> lock(m_mtx_strategies);
	return m_bCacheValid;
}

bool CStrategyService::ValidateStrategy(const request::StrategyInfo& strategy, std::string& error)
{
	error.clear();
	std::function<bool(const std::string&)> isBlank = [](const std::string& value)
	{
		return value.empty() || std::all_of(value.begin(), value.end(), [](unsigned char character)
		{
			return 0 != std::isspace(character);
		});
	};
	if (isBlank(strategy.strategy_name()) || isBlank(strategy.strategy_type()))
	{
		error = "策略名称和类型不能为空";
		return false;
	}
	if (0 == strategy.subscriptions_size())
	{
		error = "至少需要一个订阅标的";
		return false;
	}
	for (const auto& subscription : strategy.subscriptions())
	{
		if (isBlank(subscription.security()) || isBlank(subscription.exchange()) || isBlank(subscription.channel()))
		{
			error = "订阅的证券、交易所和通道不能为空";
			return false;
		}
	}
	return true;
}

void CStrategyService::OnRequestReply(const CRequest& response)
{
	std::string strCmd = response.GetCmd();
	std::optional<std::pair<int, std::string>> errorInfo = response.GetErrorInfo();
	std::string strError = (errorInfo.has_value() && (0 != errorInfo->first)) ? (errorInfo->second.empty() ? "策略请求失败" : errorInfo->second) : std::string();
	const _TyReqData& message = response.GetData();
	if ("strategy_query" == strCmd)
	{
		_TyStrategyList strategies;
		if (strError.empty() && !message.has_strategy_list())
		{
			strError = "策略查询响应缺少 strategy_list";
		}
		if (strError.empty() && message.has_strategy_list())
		{
			strategies.reserve(message.strategy_list().strategies_size());
			for (const request::StrategyInfo& strategy : message.strategy_list().strategies())
			{
				strategies.emplace_back(strategy);
			}
		}
		if (strError.empty())
		{
			std::unique_lock<std::shared_mutex> lock(m_mtx_strategies);
			m_strategies = strategies;
			m_bCacheValid = true;
		}
		else
		{
			std::unique_lock<std::shared_mutex> lock(m_mtx_strategies);
			m_bCacheValid = false;
		}
		_TyQueryHandler handler;
		{
			std::lock_guard<std::mutex> lock(m_mtx_handlers);
			handler = m_queryHandler;
		}
		if (handler)
		{
			handler(strategies, strError);
		}
		m_queryPump.Notify({ std::move(strategies), std::move(strError) });
		return;
	}

	if (("strategy_add" != strCmd) && ("strategy_modify" != strCmd) && ("strategy_delete" != strCmd))
	{
		return;
	}
	request::StrategyInfo strategy;
	if (message.has_strategy())
	{
		strategy.CopyFrom(message.strategy());
	}
	if (strError.empty())
	{
		// Mutation acknowledgements may omit the complete configuration or deleted ID.
		// Keep the last list, but require a fresh query before treating it as current.
		std::unique_lock<std::shared_mutex> lock(m_mtx_strategies);
		m_bCacheValid = false;
	}
	_TyOperationHandler handler;
	{
		std::lock_guard<std::mutex> lock(m_mtx_handlers);
		handler = m_operationHandler;
	}
	if (handler)
	{
		handler(strCmd, strError.empty(), strategy, strError);
	}
	if (strError.empty())
	{
		QueryStrategies();
	}
	else
	{
		m_queryPump.Notify({ GetStrategies(), strError });
	}
}
