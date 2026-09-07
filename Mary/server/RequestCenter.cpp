#include "RequestCenter.h"

namespace request
{
	CRequest Auth(AuthAction action, const std::string& account, const std::string& password)
	{
		CRequest req;
		req.SetType(AuthAction::Login == action ? CRequest::Type::QUERY_AUTH : CRequest::Type::UPDATE_AUTH);
		req.SetCmd(AuthAction::Login == action ? "auth" : "register");
		req.SetExtraData("user", account);
		req.SetExtraData("password", password);
		return req;
	}

	CRequest Auth(const std::string& token)
	{
		CRequest req;
		req.SetType(CRequest::Type::QUERY_AUTH);
		req.SetCmd("auth");
		req.SetExtraData("token", token);
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
