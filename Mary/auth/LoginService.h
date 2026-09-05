#pragma once

#include "../basic/CallbackRegistry.h"
#include <functional>
#include <memory>
#include <string>

struct CLoginSite
{
	std::string id;
	std::string name;
	std::string host;
	int port{0};
};

struct CLoginParam
{
	std::string account;
	std::string password;
	CLoginSite site;
};

enum class AuthState
{
	Idle,
	Connecting,
	Authenticating,
	Success,
	Failed,
	Cancelled
};

enum class AuthError
{
	None,
	InvalidInput,
	InvalidSite,
	ConnectFailed,
	AuthenticationFailed,
	NetworkError,
	Cancelled
};

struct AuthEvent
{
	AuthState state{AuthState::Idle};
	AuthError error{AuthError::None};
	bool success{false}; 
	std::string message;
};

class AuthSession;

class LoginService final
{
public:
	using Callback = std::function<void(const AuthEvent&)>;

	LoginService();
	~LoginService();
	LoginService(const LoginService&) = delete;
	LoginService& operator=(const LoginService&) = delete;

	_TyCallbackId Subscribe(Callback callback);
	void Unsubscribe(_TyCallbackId id);
	void Login(const CLoginParam& param);
	void Login(const CLoginParam& param, Callback callback);
	void Cancel();
	bool IsLoggingIn() const noexcept;

private:
	std::unique_ptr<AuthSession> m_session;
	CallbackRegistry<AuthEvent> m_events;
	bool m_loggingIn{false};
};
