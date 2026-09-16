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
	CSession::InstanceRef().RegisterResponseHandler([this](const SessionResponse& response)
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

void CStrategyService::OnResponse(const SessionResponse& response)
{
	if ("strategy_query" == response.m_cmd)
	{
		StrategyList strategies;
		if (response.m_error.empty() && response.m_message.has_strategy_list())
		{
			strategies.reserve(response.m_message.strategy_list().strategies_size());
			for (const request::StrategyInfo& strategy : response.m_message.strategy_list().strategies())
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
			handler(strategies, response.m_error);
		}
		return;
	}

	if (("strategy_add" != response.m_cmd) && ("strategy_modify" != response.m_cmd) && ("strategy_delete" != response.m_cmd))
	{
		return;
	}
	request::StrategyInfo strategy;
	if (response.m_message.has_strategy())
	{
		strategy.CopyFrom(response.m_message.strategy());
	}
	OperationHandler handler;
	{
		std::lock_guard<std::mutex> lock(m_mtx_handlers);
		handler = m_operationHandler;
	}
	if (nullptr != handler)
	{
		handler(response.m_cmd, response.m_error.empty(), strategy, response.m_error);
	}
}
