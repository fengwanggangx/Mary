#pragma once

#include "../basic/CallbackRegistry.h"
#include <functional>
#include <string>

struct CLoginParam 
{ 
	std::string account;
	std::string password; 
};

struct LoginEvent 
{ 
	bool success{false}; 
	std::string message; 
};

class LoginService final
{
public:
	LoginService() = default;
	_TyCallbackId Subscribe(std::function<void(const LoginEvent&)> callback);
	void Unsubscribe(_TyCallbackId id);
	void Login(const CLoginParam& param);

private:
	CallbackRegistry<LoginEvent> m_events;
};
