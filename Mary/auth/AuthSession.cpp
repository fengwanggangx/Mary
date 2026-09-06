#include "AuthSession.h"

#include "../network/CTcpClient.h"
#include "../request/request.h"

#include <utility>

AuthSession::~AuthSession()
{
	Cancel();
}

void AuthSession::Start(const CLoginParam& param, Callback callback)
{
	Cancel();
	m_param = param;
	m_callback = std::move(callback);
	m_running = true;

	m_client = std::make_unique<net::CTcpClient>(m_param.m_host.m_strHost, static_cast<int>(m_param.m_host.m_nPort));
	m_client->RegisterHandler([this](const net::CNetEvent& event)
	{
		OnNetworkEvent(event);
		return 1;
	});

	Notify({AuthState::Connecting, AuthError::None, false, "正在连接站点"});
	if (0 != m_client->Initialize())
	{
		Notify({AuthState::Failed, AuthError::ConnectFailed, false, "连接站点失败"});
		Cancel();
		return;
	}
	m_client->Start(true);
}

void AuthSession::Cancel()
{
	if (!m_running && (nullptr == m_client))
	{
		return;
	}
	if (nullptr != m_client)
	{
		m_client->Release();
		m_client.reset();
	}
	m_running = false;
	m_callback = nullptr;
}

bool AuthSession::IsRunning() const noexcept
{
	return m_running;
}

void AuthSession::OnNetworkEvent(const net::CNetEvent& event)
{
	if (!m_running)
	{
		return;
	}

	if (net::em_event::connected == event.m_event)
	{
		Notify({AuthState::Authenticating, AuthError::None, false, "正在认证"});
		SendAuthentication();
		return;
	}

	if (net::em_event::request == event.m_event && (nullptr != event.m_request))
	{
		const std::string command = event.m_request->GetCmd();
		if ("auth" != command)
		{
			return;
		}

		const std::string error = event.m_request->GetReturnData("error_message");
		if (error.empty())
		{
			Notify({AuthState::Success, AuthError::None, true, "登录成功"});
		}
		else
		{
			Notify({AuthState::Failed, AuthError::AuthenticationFailed, false, "账号或密码错误，请重新输入"});
		}
		m_param.m_strPassword.clear();
		m_running = false;
		return;
	}

	Notify({AuthState::Failed, AuthError::NetworkError, false, "网络连接已断开"});
	m_param.m_strPassword.clear();
	m_running = false;
}

void AuthSession::SendAuthentication()
{
	CRequest request;
	request.SetType(CRequest::Type::QUERY_AUTH);
	request.SetCmd("auth");
	request.SetExtraData("user", m_param.m_strAccount);
	request.SetExtraData("password", m_param.m_strPassword);
	if (!m_client->SendRequest(request))
	{
		Notify({AuthState::Failed, AuthError::NetworkError, false, "认证请求发送失败"});
		m_param.m_strPassword.clear();
		m_running = false;
	}
}

void AuthSession::Notify(AuthEvent event)
{
	if (m_callback)
	{
		m_callback(event);
	}
}
