#include "LoginService.h"
#include "AuthSession.h"

LoginService::LoginService() : m_session(std::make_unique<AuthSession>())
{
}

LoginService::~LoginService()
{
	Cancel();
}

_TyCallbackId LoginService::Subscribe(_TyCallback callback)
{
	return m_events.Subscribe(std::move(callback));
}

void LoginService::Unsubscribe(_TyCallbackId id)
{
	m_events.Unsubscribe(id);
}

void LoginService::Login(const CLoginParam& param)
{
	Login(param, _TyCallback{});
}

void LoginService::Login(const CLoginParam& param, _TyCallback callback)
{
	if (m_loggingIn)
	{
		return;
	}

	if (param.account.empty() || param.password.empty())
	{
		m_events.Notify({AuthState::Failed, AuthError::InvalidInput, false, "账号和密码不能为空"});
		return;
	}

	if (param.site.id.empty() || param.site.host.empty() || (0 >= param.site.port) || (65535 < param.site.port))
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

void LoginService::Cancel()
{
	if (nullptr != m_session)
	{
		m_session->Cancel();
	}
	m_loggingIn = false;
}

bool LoginService::IsLoggingIn() const noexcept
{
	return m_loggingIn;
}
