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
		hqmarket::market::v1::SubscribeRequest payload;
		CSecurity security = ParseSecurity(req.GetExtraData("security"));
		payload.add_securities()->set_symbol(security.m_strCode);
		payload.mutable_securities(0)->set_exchange(static_cast<hqmarket::market::v1::Exchange>(security.m_market));
		payload.add_channels(static_cast<hqmarket::market::v1::Channel>(ParseChannel(req.GetExtraData("channel"))));
		req.SetData(payload);
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
		hqmarket::market::v1::UnsubscribeRequest payload;
		CSecurity security = ParseSecurity(req.GetExtraData("security"));
		payload.add_securities()->set_symbol(security.m_strCode);
		payload.mutable_securities(0)->set_exchange(static_cast<hqmarket::market::v1::Exchange>(security.m_market));
		payload.add_channels(static_cast<hqmarket::market::v1::Channel>(ParseChannel(req.GetExtraData("channel"))));
		req.SetData(payload);
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
		hqmarket::market::v1::QueryRequest payload;
		payload.mutable_security()->set_symbol(info.m_strCode);
		payload.mutable_security()->set_exchange(static_cast<hqmarket::market::v1::Exchange>(info.m_market));
		payload.set_channel(static_cast<hqmarket::market::v1::Channel>(ParseChannel(strChannel)));
		payload.set_begin_time_ms(nBeginTime);
		payload.set_end_time_ms(nEndTime);
		req.SetData(payload);
		return req;
	}

	CRequest QueryMarketSecurities()
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("query_securities");
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
