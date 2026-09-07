#pragma once

#include "../basic/CallbackRegistry.h"
#include "../common/ISingleton.h"
#include "CHostMgr.h"

#include <atomic>
#include <functional>
#include <string>

struct CLoginParam
{
	std::string m_strAccount;
	std::string m_strPassword;
	CHostInfo m_host;
};

struct CRegisterParam
{
	std::string m_strAccount;
	std::string m_strPassword;
	CHostInfo m_host;
};

enum class AuthOperation
{
	Login,
	Register
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
	AuthOperation m_operation{ AuthOperation::Login };
	AuthState m_state{ AuthState::Idle };
	AuthError m_error{ AuthError::None };
	bool m_success{ false };
	std::string m_message;
};

class CLoginService final : public ISingleton<CLoginService>
{
	DECLARE_SINGLE_DFAULT(CLoginService)

public:
	using _TyCallback = std::function<void(const AuthEvent&)>;

	_TyCallbackId Subscribe(_TyCallback&& callback);
	void Unsubscribe(_TyCallbackId id);
	void Login(const CLoginParam& param);
	void Register(const CRegisterParam& param);
	void Cancel();
	bool IsBusy() const noexcept;

private:
	void Start(AuthOperation operation, const CLoginParam& param);

	CallbackRegistry<AuthEvent> m_events;
	std::atomic_bool m_busy{ false };
};
