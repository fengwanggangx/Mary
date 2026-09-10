#include "RequestCenter.h"

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

	CRequest Subscription(const RequestParameters& param)
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("subscribe");
		for (const auto& [strKey, strValue] : param)
		{
			req.SetExtraData(strKey, strValue);
		}
		return req;
	}

	CRequest UnSubscription(const RequestParameters& param)
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("unsubscribe");
		for (const auto& [strKey, strValue] : param)
		{
			req.SetExtraData(strKey, strValue);
		}
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
}
