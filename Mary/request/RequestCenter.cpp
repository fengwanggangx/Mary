#include "RequestCenter.h"
#include "v1/market.pb.h"

#include <chrono>

namespace request
{
	CRequest Auth(AuthAction action, const std::string& strAccount, const std::string& strPassword)
	{
		CRequest req;
		req.SetType(AuthAction::Login == action ? CRequest::Type::QUERY_AUTH : CRequest::Type::UPDATE_AUTH);
		req.SetCmd(AuthAction::Login == action ? "auth" : "register");
		req.SetExtraData("user", strAccount);
		req.SetExtraData("password", strPassword);
		return req;
	}

	CRequest Auth(const std::string& strToken)
	{
		CRequest req;
		req.SetType(CRequest::Type::QUERY_AUTH);
		req.SetCmd("auth");
		req.SetExtraData("token", strToken);
		return req;
	}

	CRequest Subscription(const _TyParams& param)
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("subscribe");
		for (const auto& [k, v] : param)
		{
			req.SetExtraData(k, v);
		}
		return req;
	}

	CRequest UnSubscription(const _TyParams& param)
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("unsubscribe");
		for (const auto& [k, v] : param)
		{
			req.SetExtraData(k, v);
		}
		return req;
	}

	CRequest QueryMarketBars(const CSecurity& info, const std::string& strChannel, std::int64_t nBeginTime, std::int64_t nEndTime)
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("query_bars");
		req.SetExtraData("security", info.String());
		req.SetExtraData("channel", strChannel);
		req.SetExtraData("begin_time_ms", std::to_string(nBeginTime));
		req.SetExtraData("end_time_ms", std::to_string(nEndTime));
		return req;
	}

	CRequest QueryMarketInstruments()
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("query_instruments");
		hqmarket::market::v1::InstrumentListRequest value;
		req.SetData(value);
		return req;
	}

	CRequest AddStrategy(const _TyStrategyInfo& strategy)
	{
		CRequest req;
		req.SetType(CRequest::Type::STRATEGY);
		req.SetCmd("strategy_add");
		req.SetData(strategy);
		return req;
	}

	CRequest ModifyStrategy(const _TyStrategyInfo& strategy)
	{
		CRequest req;
		req.SetType(CRequest::Type::STRATEGY);
		req.SetCmd("strategy_modify");
		req.SetData(strategy);
		return req;
	}

	CRequest QueryStrategies()
	{
		CRequest req;
		req.SetType(CRequest::Type::STRATEGY);
		req.SetCmd("strategy_query");
		return req;
	}

	CRequest DeleteStrategy(std::uint64_t id)
	{
		CRequest req;
		req.SetType(CRequest::Type::STRATEGY);
		req.SetCmd("strategy_delete");
		req.SetExtraData("strategy_id", std::to_string(id));
		return req;
	}

	CRequest HeartBeat()
	{
		CRequest req;
		req.SetType(CRequest::Type::HEARTBEAT);
		req.SetCmd("heartbeat");
		std::int64_t nClientTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		req.SetExtraData("client_time_ms", std::to_string(nClientTime));
		return req;
	}
} // namespace request
