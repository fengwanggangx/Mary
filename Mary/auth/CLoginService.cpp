#include "CLoginService.h"
#include "AuthSession.h"

CLoginService::CLoginService() : m_session(std::make_unique<AuthSession>())
{
}

CLoginService::~CLoginService()
{
	Cancel();
}

_TyCallbackId CLoginService::Subscribe(_TyCallback callback)
{
	return m_events.Subscribe(std::move(callback));
}

void CLoginService::Unsubscribe(_TyCallbackId id)
{
	m_events.Unsubscribe(id);
}

void CLoginService::Login(const CLoginParam& param)
{
	Login(param, _TyCallback{});
}

void CLoginService::Login(const CLoginParam& param, _TyCallback callback)
{
	Start(AuthOperation::Login, param, std::move(callback));
}

void CLoginService::Register(const CRegisterParam& param)
{
	Register(param, _TyCallback{});
}

void CLoginService::Register(const CRegisterParam& param, _TyCallback callback)
{
	CLoginParam sessionParam;
	sessionParam.m_strAccount = param.m_strAccount;
	sessionParam.m_strPassword = param.m_strPassword;
	sessionParam.m_host = param.m_host;
	Start(AuthOperation::Register, sessionParam, std::move(callback));
}

void CLoginService::Start(AuthOperation operation, const CLoginParam& param, _TyCallback callback)
{
	if (m_busy)
	{
		return;
	}

	if (param.m_strAccount.empty() || param.m_strPassword.empty())
	{
		m_events.Notify({ operation, AuthState::Failed, AuthError::InvalidInput, false, "账号和密码不能为空" });
		return;
	}

	if (!param.m_host.Valid())
	{
		m_events.Notify({ operation, AuthState::Failed, AuthError::InvalidSite, false, "当前站点配置无效" });
		return;
	}

	m_busy = true;
	m_session->Start(operation, param, [this, callback = std::move(callback)](const AuthEvent& event)
	{
		m_events.Notify(event);
		if (callback)
		{
			callback(event);
		}
		if (AuthState::Success == event.m_state || AuthState::Failed == event.m_state || AuthState::Cancelled == event.m_state)
		{
			m_busy = false;
		}
	});
}

void CLoginService::Cancel()
{
	if (nullptr != m_session)
	{
		m_session->Cancel();
	}
	m_busy = false;
}

bool CLoginService::IsBusy() const noexcept
{
	return m_busy;
}
