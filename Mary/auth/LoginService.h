#pragma once

#include "../basic/CallbackRegistry.h"
#include <functional>
#include <memory>
#include <string>

struct LoginRequest 
{ 
	std::string account;
	std::string password; 
};

struct LoginEvent 
{ 
	bool success{false}; 
	std::string message; 
};

class ILoginClient
{
public:
	using Callback = std::function<void(const LoginEvent&)>;
	virtual ~ILoginClient() = default;
	virtual void Login(const LoginRequest& request, Callback callback) = 0;
};

class LoginService final
{
public:
	explicit LoginService(std::unique_ptr<ILoginClient> client);
	CallbackId Subscribe(std::function<void(const LoginEvent&)> callback);
	void Unsubscribe(CallbackId id);
	void Login(const LoginRequest& request);

private:
	std::unique_ptr<ILoginClient> m_client;
	CallbackRegistry<LoginEvent> m_events;
};

std::unique_ptr<ILoginClient> MakeLocalLoginClient();
