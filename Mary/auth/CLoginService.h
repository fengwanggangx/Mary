#pragma once

#include "../basic/CallbackRegistry.h"
#include "../common/ISingleton.h"
#include "../configuration/CHostMgr.h"

#include <functional>
#include <memory>
#include <string>

struct CLoginParam
{
	std::string m_strAccount;
	std::string m_strPassword;
	CHostInfo m_host;
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

class CLoginService final : public ISingleton<CLoginService>
{
	DECLARE_SINGLE_DFAULT(CLoginService)

public:
	using _TyCallback = std::function<void(const AuthEvent&)>;

	_TyCallbackId Subscribe(_TyCallback callback);
	void Unsubscribe(_TyCallbackId id);
	void Login(const CLoginParam& param);
	void Login(const CLoginParam& param, _TyCallback callback);
	void Cancel();
	bool IsLoggingIn() const noexcept;

private:
	std::unique_ptr<AuthSession> m_session;
	CallbackRegistry<AuthEvent> m_events;
	bool m_loggingIn{false};
};
