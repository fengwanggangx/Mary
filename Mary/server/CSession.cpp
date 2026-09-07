#include "CSession.h"

#include "../network/CTcpClient.h"

#include <algorithm>
#include <utility>
#include <vector>

CSession::CSession() = default;

CSession::~CSession()
{
	Stop();
}

void CSession::Authenticate(AuthOperation op, const CLoginParam& param, AuthCallback&& cb)
{
	bool bRestart = false;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		bRestart = m_connectionThread.joinable() && !(m_host == param.m_host);
	}
	if (bRestart)
	{
		Stop();
	}

	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		m_authOperation = op;
		m_authParam = param;
		m_authCallback = std::move(cb);
		m_authRequested = true;
		m_host = param.m_host;
	}

	SessionState state = m_state.load();
	if (!m_connectionThread.joinable())
	{
		StartConnection();
	}
	else if ((SessionState::Connected == state) || (SessionState::Ready == state) || (SessionState::Authenticating == state))
	{
		SendAuthentication();
	}
}

void CSession::CancelAuthentication()
{
	AuthCallback cb;
	AuthEvent event;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		if (!m_authRequested)
		{
			return;
		}
		event = { m_authOperation, AuthState::Cancelled, AuthError::Cancelled, false, "认证已取消" };
		cb = m_authCallback;
		m_authRequested = false;
		m_authCallback = nullptr;
		m_authParam.m_strPassword.clear();
	}
	if (nullptr != cb)
	{
		cb(event);
	}
}

void CSession::StartConnection()
{
	m_stopping.store(false);
	m_connectionThread = std::thread(&CSession::ConnectionLoop, this);
	m_maintenanceThread = std::thread(&CSession::MaintenanceLoop, this);
}

void CSession::Stop()
{
	if (!m_connectionThread.joinable() && !m_maintenanceThread.joinable())
	{
		return;
	}

	m_stopping.store(true);
	m_state.store(SessionState::Stopping);
	m_cv_loops.notify_all();
	{
		std::lock_guard<std::mutex> lock(m_mtx_client);
		if (nullptr != m_client)
		{
			m_client->ShutDown();
		}
	}
	if (m_maintenanceThread.joinable())
	{
		m_maintenanceThread.join();
	}
	if (m_connectionThread.joinable())
	{
		m_connectionThread.join();
	}
	FailPending("客户端已关闭");
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		m_authParam.m_strPassword.clear();
		m_authRequested = false;
		m_authCallback = nullptr;
	}
	m_state.store(SessionState::Disconnected);
}

bool CSession::IsAuthenticated() const noexcept
{
	return SessionState::Ready == m_state.load();
}

SessionState CSession::GetState() const noexcept
{
	return m_state.load();
}

bool CSession::Subscribe(const std::string& strKey, const request::RequestParameters& param)
{
	if (strKey.empty())
	{
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		m_desiredSubscriptions.insert_or_assign(strKey, Subscription{ param });
	}
	return (0 != SendRequest(request::Subscription(param)));
}

bool CSession::Unsubscribe(const std::string& strKey, const request::RequestParameters& param)
{
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		if (0 == m_desiredSubscriptions.erase(strKey))
		{
			return false;
		}
	}
	return !IsAuthenticated() || (0 != SendRequest(request::UnSubscription(param)));
}

void CSession::SetStateCallback(StateCallback&& cb)
{
	std::lock_guard<std::mutex> lock(m_mtx_callbacks);
	m_stateCallback = std::move(cb);
}

void CSession::SetResponseCallback(ResponseCallback&& cb)
{
	std::lock_guard<std::mutex> lock(m_mtx_callbacks);
	m_responseCallback = std::move(cb);
}

void CSession::SetErrorCallback(ErrorCallback&& cb)
{
	std::lock_guard<std::mutex> lock(m_mtx_callbacks);
	m_errorCallback = std::move(cb);
}

void CSession::ConnectionLoop()
{
	int reconnectSeconds = 1;
	while (!m_stopping.load())
	{
		SessionState state = 1 == reconnectSeconds ? SessionState::Connecting : SessionState::Reconnecting;
		m_state.store(state);
		NotifyState(state, 1 == reconnectSeconds ? "正在连接" : std::to_string(reconnectSeconds) + " 秒后重连");

		CHostInfo host;
		{
			std::lock_guard<std::mutex> lock(m_mtx_auth);
			host = m_host;
		}
		std::unique_ptr<net::CTcpClient> client = std::make_unique<net::CTcpClient>(host.m_strHost, static_cast<int>(host.m_nPort));
		client->RegisterHandler([this](const net::CNetEvent& event)
		{
			OnNetEvent(event);
			return 1;
		});
		{
			std::lock_guard<std::mutex> lock(m_mtx_client);
			m_client = std::move(client);
		}
		int result = 0;
		{
			std::lock_guard<std::mutex> lock(m_mtx_client);
			result = m_client->Initialize();
		}
		if (0 == result)
		{
			m_client->Start(true);
		}
		else
		{
			NotifyError("连接初始化失败：" + std::to_string(result));
		}

		bool wasAuthenticated = IsAuthenticated();
		m_state.store(SessionState::Disconnected);
		FailPending("连接已断开");
		{
			std::lock_guard<std::mutex> lock(m_mtx_client);
			m_client.reset();
		}
		if (m_stopping.load())
		{
			break;
		}
		std::unique_lock<std::mutex> lock(m_mtx_wait);
		m_cv_loops.wait_for(lock, std::chrono::seconds(reconnectSeconds), [this]()
		{
			return m_stopping.load();
		});
		reconnectSeconds = wasAuthenticated ? 1 : (std::min)(m_maxReconnectSeconds, reconnectSeconds * 2);
	}
	NotifyState(SessionState::Disconnected, "已关闭");
}

void CSession::MaintenanceLoop()
{
	std::chrono::steady_clock::time_point nextHeartbeat = std::chrono::steady_clock::now();
	while (!m_stopping.load())
	{
		std::unique_lock<std::mutex> waitLock(m_mtx_wait);
		m_cv_loops.wait_for(waitLock, std::chrono::milliseconds(250), [this]()
		{
			return m_stopping.load();
		});
		waitLock.unlock();
		if (m_stopping.load())
		{
			break;
		}

		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (IsAuthenticated() && (nextHeartbeat <= now))
		{
			SendRequest(request::HeartBeat());
			nextHeartbeat = now + std::chrono::seconds(m_heartbeatSeconds);
		}
		std::vector<SessionResponse> expired;
		{
			std::lock_guard<std::mutex> lock(m_mtx_pending);
			for (auto mIter = m_reqs_pending.begin(); m_reqs_pending.end() != mIter;)
			{
				if (mIter->second.m_deadline <= now)
				{
					expired.push_back({ mIter->first, mIter->second.m_cmd, {}, "请求超时" });
					mIter = m_reqs_pending.erase(mIter);
				}
				else
				{
					++mIter;
				}
			}
		}
		for (SessionResponse& response : expired)
		{
			NotifyResponse(std::move(response));
		}
	}
}

void CSession::OnNetEvent(const net::CNetEvent& ev)
{
	if (net::em_event::connected == ev.m_event)
	{
		m_state.store(SessionState::Connected);
		NotifyState(SessionState::Connected, "已连接");
		SendAuthentication();
		return;
	}
	if ((net::em_event::request == ev.m_event) && (nullptr != ev.m_request))
	{
		HandleResponse(*ev.m_request);
		return;
	}

	bool bAuthed = IsAuthenticated();
	m_state.store(SessionState::Disconnected);
	NotifyState(SessionState::Disconnected, "连接已断开");
	NotifyError(0 == ev.m_error ? "服务器主动关闭连接" : "网络错误：" + std::to_string(ev.m_error));
	if (bAuthed)
	{
		NotifyState(SessionState::Disconnected, "认证连接已断开");
	}
}

void CSession::HandleResponse(const CRequest& response)
{
	std::uint64_t id = response.GetId();
	std::string cmd = response.GetCmd();
	request::RequestParameters result = response.GetReturnData();
	std::string error;
	auto errorIter = result.find("error_message");
	if (result.end() != errorIter)
	{
		error = errorIter->second;
	}

	if ((CRequest::Type::QUERY_AUTH == response.GetType()) || (CRequest::Type::UPDATE_AUTH == response.GetType()))
	{
		AuthOperation op = CRequest::Type::QUERY_AUTH == response.GetType() ? AuthOperation::Login : AuthOperation::Register;
		if (error.empty())
		{
			if (AuthOperation::Login == op)
			{
				m_state.store(SessionState::Ready);
				NotifyState(SessionState::Ready, "已认证");
				RestoreSubscriptions();
			}
			else
			{
				m_state.store(SessionState::Connected);
			}
			NotifyAuthentication({ op, AuthState::Success, AuthError::None, true, AuthOperation::Login == op ? "登录成功" : "注册成功" });
		}
		else
		{
			m_state.store(SessionState::Connected);
			NotifyAuthentication({ op, AuthState::Failed, AuthError::AuthenticationFailed, false, error });
		}
		return;
	}

	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		m_reqs_pending.erase(id);
	}
	NotifyResponse({ id, std::move(cmd), std::move(result), std::move(error) });
}

void CSession::SendAuthentication()
{
	AuthOperation op;
	CLoginParam param;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		if (!m_authRequested)
		{
			return;
		}
		op = m_authOperation;
		param = m_authParam;
	}
	m_state.store(SessionState::Authenticating);
	NotifyAuthentication({ op, AuthState::Authenticating, AuthError::None, false, AuthOperation::Login == op ? "正在认证" : "正在注册" });

	request::AuthAction action = AuthOperation::Login == op ? request::AuthAction::Login : request::AuthAction::Register;
	if (0 == SendRequest(request::Auth(action, param.m_strAccount, param.m_strPassword)))
	{
		NotifyAuthentication({ op, AuthState::Failed, AuthError::NetworkError, false, "认证请求发送失败" });
	}
}

bool CSession::SendRequest(const CRequest& req)
{
	CRequest::Type t = req.GetType();
	bool bAuth = (CRequest::Type::QUERY_AUTH == t) || (CRequest::Type::UPDATE_AUTH == t);
	if (!bAuth && !IsAuthenticated())
	{
		return false;
	}

	std::uint64_t id = req.GetId();
	std::string strCmd = req.GetCmd();
	if (!bAuth)
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		m_reqs_pending.emplace(id, PendingRequest{ strCmd, std::chrono::steady_clock::now() + std::chrono::seconds(m_timeoutSeconds) });
	}
	bool bRet = false;
	{
		std::lock_guard<std::mutex> lock(m_mtx_client);
		bRet = (nullptr != m_client) && m_client->SendRequest(req);
	}
	if (!bRet)
	{
		if (!bAuth)
		{
			std::lock_guard<std::mutex> lock(m_mtx_pending);
			m_reqs_pending.erase(id);
		}
	}
	return bRet;
}

void CSession::RestoreSubscriptions()
{
	std::unordered_map<std::string, Subscription> subscriptions;
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		subscriptions = m_desiredSubscriptions;
	}
	for (const auto& [k, v] : subscriptions)
	{
		SendRequest(request::Subscription(v.m_param));
	}
}

void CSession::FailPending(const std::string& reason)
{
	std::vector<SessionResponse> failed;
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		failed.reserve(m_reqs_pending.size());
		for (const auto& [id, pending] : m_reqs_pending)
		{
			failed.push_back({ id, pending.m_cmd, {}, reason });
		}
		m_reqs_pending.clear();
	}
	for (SessionResponse& response : failed)
	{
		NotifyResponse(std::move(response));
	}
}

void CSession::NotifyAuthentication(AuthEvent ev)
{
	AuthCallback cb;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		cb = m_authCallback;
		if ((AuthState::Success == ev.m_state) || (AuthState::Failed == ev.m_state) || (AuthState::Cancelled == ev.m_state))
		{
			m_authCallback = nullptr;
			m_authRequested = AuthState::Success == ev.m_state && AuthOperation::Login == ev.m_operation;
			if (!m_authRequested)
			{
				m_authParam.m_strPassword.clear();
			}
		}
	}
	if (nullptr != cb)
	{
		cb(ev);
	}
}

void CSession::NotifyState(SessionState state, const std::string& message)
{
	StateCallback cb;
	{
		std::lock_guard<std::mutex> lock(m_mtx_callbacks);
		cb = m_stateCallback;
	}
	if (nullptr != cb)
	{
		cb(state, message);
	}
}

void CSession::NotifyResponse(SessionResponse response)
{
	ResponseCallback cb;
	{
		std::lock_guard<std::mutex> lock(m_mtx_callbacks);
		cb = m_responseCallback;
	}
	if (nullptr != cb)
	{
		cb(response);
	}
}

void CSession::NotifyError(const std::string& error)
{
	ErrorCallback cb;
	{
		std::lock_guard<std::mutex> lock(m_mtx_callbacks);
		cb = m_errorCallback;
	}
	if (nullptr != cb)
	{
		cb(error);
	}
}
