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
	if (m_loggingIn)
	{
		return;
	}

	if (param.m_strAccount.empty() || param.m_strPassword.empty())
	{
		m_events.Notify({AuthState::Failed, AuthError::InvalidInput, false, "账号和密码不能为空"});
		return;
	}

	if (!param.m_host.Valid())
	{
		m_events.Notify({AuthState::Failed, AuthError::InvalidSite, false, "当前站点配置无效"});
		return;
	}

	m_loggingIn = true;
	m_session->Start(param, [this, callback = std::move(callback)](const AuthEvent& event)
	{
		m_events.Notify(event);
		if (callback)
		{
			callback(event);
		}
		if (AuthState::Success == event.state || AuthState::Failed == event.state || AuthState::Cancelled == event.state)
		{
			m_loggingIn = false;
		}
	});
}

void CLoginService::Cancel()
{
	if (nullptr != m_session)
	{
		m_session->Cancel();
	}
	m_loggingIn = false;
}

bool CLoginService::IsLoggingIn() const noexcept
{
	return m_loggingIn;
}
