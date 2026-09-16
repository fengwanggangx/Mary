#include "CStrategyService.h"

#include "CSession.h"

#include <utility>

CStrategyService::CStrategyService() = default;

CStrategyService::~CStrategyService() = default;

void CStrategyService::Initialize()
{
	if (m_bInitialized)
	{
		return;
	}
	m_bInitialized = true;
	CSession::InstanceRef().RegisterResponseHandler([this](const CRequest& response)
	{
		OnResponse(response);
	});
}

void CStrategyService::SetQueryHandler(QueryHandler&& handler)
{
	std::lock_guard<std::mutex> lock(m_mtx_handlers);
	m_queryHandler = std::move(handler);
}

void CStrategyService::SetOperationHandler(OperationHandler&& handler)
{
	std::lock_guard<std::mutex> lock(m_mtx_handlers);
	m_operationHandler = std::move(handler);
}

bool CStrategyService::QueryStrategies()
{
	return CSession::InstanceRef().SendRequest(request::QueryStrategies());
}

bool CStrategyService::AddStrategy(const request::StrategyInfo& strategy)
{
	return CSession::InstanceRef().SendRequest(request::AddStrategy(strategy));
}

bool CStrategyService::ModifyStrategy(const request::StrategyInfo& strategy)
{
	return CSession::InstanceRef().SendRequest(request::ModifyStrategy(strategy));
}

bool CStrategyService::DeleteStrategy(std::uint64_t id)
{
	return CSession::InstanceRef().SendRequest(request::DeleteStrategy(id));
}

void CStrategyService::OnResponse(const CRequest& response)
{
	std::string strCmd = response.GetCmd();
	std::optional<std::pair<int, std::string>> errorInfo = response.GetErrorInfo();
	std::string strError = errorInfo.has_value() ? errorInfo->second : std::string();
	const _TyReqData& message = response.GetData();
	if ("strategy_query" == strCmd)
	{
		StrategyList strategies;
		if (strError.empty() && message.has_strategy_list())
		{
			strategies.reserve(message.strategy_list().strategies_size());
			for (const request::StrategyInfo& strategy : message.strategy_list().strategies())
			{
				strategies.emplace_back(strategy);
			}
		}
		QueryHandler handler;
		{
			std::lock_guard<std::mutex> lock(m_mtx_handlers);
			handler = m_queryHandler;
		}
		if (nullptr != handler)
		{
			handler(strategies, strError);
		}
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
	OperationHandler handler;
	{
		std::lock_guard<std::mutex> lock(m_mtx_handlers);
		handler = m_operationHandler;
	}
	if (nullptr != handler)
	{
		handler(strCmd, strError.empty(), strategy, strError);
	}
}
