#include "CLoginService.h"
#include "CSession.h"

CLoginService::CLoginService()
{
}

CLoginService::~CLoginService()
{
	Cancel();
}

_TyCallbackId CLoginService::Subscribe(_TyCallback&& callback)
{
	return m_dispatcher_auth.Subscribe(std::move(callback));
}

void CLoginService::Unsubscribe(_TyCallbackId id)
{
	m_dispatcher_auth.Unsubscribe(id);
}

bool CLoginInfo::Valid() const noexcept
{
	return !m_strAccount.empty() && !m_strToken.empty() && m_host.Valid();
}

void CLoginService::Authenticate(const CAuthParam& param)
{
	if (m_busy)
	{
		return;
	}

	if (param.m_strAccount.empty() || param.m_strPassword.empty())
	{
		m_dispatcher_auth.Notify({ param.m_operation, AuthState::Failed, AuthError::InvalidInput, false, "账号和密码不能为空" });
		return;
	}

	if (!param.m_host.Valid())
	{
		m_dispatcher_auth.Notify({ param.m_operation, AuthState::Failed, AuthError::InvalidSite, false, "当前站点配置无效" });
		return;
	}

	m_busy = true;
	CSession::InstanceRef().Authenticate(param, [this](const AuthEvent& event)
	{
		m_dispatcher_auth.Notify(event);
		if (AuthState::Success == event.m_state || AuthState::Failed == event.m_state || AuthState::Cancelled == event.m_state)
		{
			m_busy = false;
		}
	});
}

void CLoginService::Cancel()
{
	CSession::InstanceRef().CancelAuthentication();
	m_busy = false;
}

bool CLoginService::IsBusy() const noexcept
{
	return m_busy;
}
