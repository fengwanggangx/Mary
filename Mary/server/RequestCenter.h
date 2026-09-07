#ifndef MARY_SERVER_REQUESTCENTER_H
#define MARY_SERVER_REQUESTCENTER_H

#include "../request/request.h"

#include <string>
#include <unordered_map>

namespace request
{
	using RequestParameters = std::unordered_map<std::string, std::string>;

	enum class AuthAction
	{
		Login,
		Register
	};

	CRequest Auth(AuthAction action, const std::string& account, const std::string& password);
	CRequest Subscription(const RequestParameters& param);
	CRequest UnSubscription(const RequestParameters& param);
	CRequest HeartBeat();
}

#endif
