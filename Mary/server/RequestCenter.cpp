#include "RequestCenter.h"

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
		return req;
	}

	CRequest UnSubscription(const RequestParameters& param)
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("unsubscribe");
		return req;
	}

	CRequest HeartBeat()
	{
		CRequest req;
		req.SetType(CRequest::Type::HQMARKET);
		req.SetCmd("cmd");
		return req;
	}
}
