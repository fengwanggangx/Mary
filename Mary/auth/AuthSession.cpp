#include "AuthSession.h"

#include "../network/CTcpClient.h"
#include "../request/request.h"

#include <utility>

AuthSession::~AuthSession()
{
	Cancel();
}

void AuthSession::Start(AuthOperation operation, const CLoginParam& param, Callback callback)
{
	Cancel();
	m_operation = operation;
	m_param = param;
	m_callback = std::move(callback);
	m_running = true;

	m_client = std::make_unique<net::CTcpClient>(m_param.m_host.m_strHost, static_cast<int>(m_param.m_host.m_nPort));
	m_client->RegisterHandler([this](const net::CNetEvent& event)
	{
		OnNetworkEvent(event);
		return 1;
	});

	Notify({ m_operation, AuthState::Connecting, AuthError::None, false, "正在连接站点" });
	if (0 != m_client->Initialize())
	{
		Notify({ m_operation, AuthState::Failed, AuthError::ConnectFailed, false, "连接站点失败" });
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
		Notify({ m_operation, AuthState::Authenticating, AuthError::None, false, AuthOperation::Login == m_operation ? "正在认证" : "正在注册" });
		SendRequest();
		return;
	}

	if (net::em_event::request == event.m_event && (nullptr != event.m_request))
	{
		const std::string command = event.m_request->GetCmd();
		std::string expectedCommand = AuthOperation::Login == m_operation ? "auth" : "register";
		if (expectedCommand != command)
		{
			return;
		}

		const std::string error = event.m_request->GetReturnData("error_message");
		if (error.empty())
		{
			Notify({ m_operation, AuthState::Success, AuthError::None, true, AuthOperation::Login == m_operation ? "登录成功" : "注册成功" });
		}
		else
		{
			Notify({ m_operation, AuthState::Failed, AuthError::AuthenticationFailed, false, error });
		}
		m_param.m_strPassword.clear();
		m_running = false;
		return;
	}

	Notify({ m_operation, AuthState::Failed, AuthError::NetworkError, false, "网络连接已断开" });
	m_param.m_strPassword.clear();
	m_running = false;
}

void AuthSession::SendRequest()
{
	CRequest request;
	request.SetType(AuthOperation::Login == m_operation ? CRequest::Type::QUERY_AUTH : CRequest::Type::UPDATE_AUTH);
	request.SetCmd(AuthOperation::Login == m_operation ? "auth" : "register");
	request.SetExtraData("user", m_param.m_strAccount);
	request.SetExtraData("password", m_param.m_strPassword);
	if (!m_client->SendRequest(request))
	{
		Notify({ m_operation, AuthState::Failed, AuthError::NetworkError, false, AuthOperation::Login == m_operation ? "认证请求发送失败" : "注册请求发送失败" });
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
