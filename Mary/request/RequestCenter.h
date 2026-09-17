#ifndef MARY_SERVER_REQUESTCENTER_H
#define MARY_SERVER_REQUESTCENTER_H

#include "request.h"
#include "MarketTypes.h"

#include <string>
#include <unordered_map>

namespace request
{
	using _TyParams = std::unordered_map<std::string, std::string>;

	enum class AuthAction
	{
		Login,
		Register
	};

	CRequest Auth(AuthAction action, const std::string& strAccount, const std::string& strPassword);
	CRequest Auth(const std::string& strToken);
	CRequest Subscription(const _TyParams& param);
	CRequest UnSubscription(const _TyParams& param);
	CRequest QueryMarketBars(const CSecurity& info, const std::string& strChannel, std::int64_t nBeginTime, std::int64_t nEndTime);
	CRequest QueryMarketSecurities();
	CRequest AddStrategy(const _TyStrategyInfo& strategy);
	CRequest ModifyStrategy(const _TyStrategyInfo& strategy);
	CRequest QueryStrategies();
	CRequest DeleteStrategy(std::uint64_t id);
	CRequest HeartBeat();
} // namespace request

#endif
