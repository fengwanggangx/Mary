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

void CSession::Authenticate(const CAuthParam& param, AuthCallback&& cb)
{
	bool bRestart = false;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		std::optional<CHostInfo> host;
		if (m_authContext.has_value())
		{
			host = m_authContext->m_param.m_host;
		}
		else if (m_loginInfo.has_value())
		{
			host = m_loginInfo->m_host;
		}
		bRestart = m_thread_conn.joinable() && (!host.has_value() || !(host.value() == param.m_host));
	}
	if (bRestart)
	{
		Stop();
	}

	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		m_loginInfo.reset();
		m_authContext.emplace(AuthContext{ param, std::move(cb) });
	}

	SessionState state = m_state.load();
	if (!m_thread_conn.joinable())
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
	AuthEvent ev;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		if (!m_authContext.has_value())
		{
			return;
		}
		ev = { m_authContext->m_param.m_operation, AuthState::Cancelled, AuthError::Cancelled, false, "认证已取消" };
		cb = std::move(m_authContext->m_callback);
		m_authContext.reset();
	}
	if (nullptr != cb)
	{
		cb(ev);
	}
}

void CSession::StartConnection()
{
	m_stopping.store(false);
	m_thread_conn = std::thread(&CSession::ConnectionLoop, this);
	m_thread_heartbeat = std::thread(&CSession::MaintenanceLoop, this);
}

void CSession::Stop()
{
	if (!m_thread_conn.joinable() && !m_thread_heartbeat.joinable())
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
	if (m_thread_heartbeat.joinable())
	{
		m_thread_heartbeat.join();
	}
	if (m_thread_conn.joinable())
	{
		m_thread_conn.join();
	}
	FailPending("客户端已关闭");
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		m_authContext.reset();
		m_loginInfo.reset();
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

std::optional<CLoginInfo> CSession::GetLoginInfo() const
{
	std::lock_guard<std::mutex> lock(m_mtx_auth);
	return m_loginInfo;
}

bool CSession::Subscribe(const std::string& strKey, const request::RequestParameters& param)
{
	if (strKey.empty())
	{
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		m_subscriptions.insert_or_assign(strKey, Subscription{ param });
	}
	return SendRequest(request::Subscription(param));
}

bool CSession::Unsubscribe(const std::string& strKey, const request::RequestParameters& param)
{
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		if (0 == m_subscriptions.erase(strKey))
		{
			return false;
		}
	}
	return SendRequest(request::UnSubscription(param));
}

bool CSession::AddStrategy(const _TyStrategyInfo& strategy)
{
	return SendRequest(request::AddStrategy(strategy));
}

bool CSession::ModifyStrategy(const _TyStrategyInfo& strategy)
{
	return SendRequest(request::ModifyStrategy(strategy));
}

bool CSession::QueryStrategies()
{
	return SendRequest(request::QueryStrategies());
}

bool CSession::DeleteStrategy(std::uint64_t id)
{
	return SendRequest(request::DeleteStrategy(id));
}

void CSession::SetStateCallback(StateCallback&& cb)
{
	std::lock_guard<std::mutex> lock(m_mtx_callbacks);
	m_stateCallback = std::move(cb);
}

void CSession::SetResponseCallback(ResponseCallback&& cb)
{
	std::lock_guard<std::mutex> lock(m_mtx_callbacks);
	m_responseCallbacks.clear();
	m_responseCallbacks.emplace_back(std::move(cb));
}

void CSession::RegisterResponseHandler(ResponseCallback&& cb)
{
	std::lock_guard<std::mutex> lock(m_mtx_callbacks);
	m_responseCallbacks.emplace_back(std::move(cb));
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

		std::optional<CHostInfo> host;
		{
			std::lock_guard<std::mutex> lock(m_mtx_auth);
			if (m_authContext.has_value())
			{
				host = m_authContext->m_param.m_host;
			}
			else if (m_loginInfo.has_value())
			{
				host = m_loginInfo->m_host;
			}
		}
		if (!host.has_value())
		{
			break;
		}
		std::unique_ptr<net::CTcpClient> client = std::make_unique<net::CTcpClient>(host->m_strHost, static_cast<int>(host->m_nPort));
		client->RegisterHandler(std::bind_front(&CSession::OnNetEvent, this));

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

		bool bAuthed = IsAuthenticated();
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
		std::unique_lock<std::mutex> lock(m_mtx_loops);
		m_cv_loops.wait_for(lock, std::chrono::seconds(reconnectSeconds), [this]()
							{ return m_stopping.load(); });
		reconnectSeconds = bAuthed ? 1 : (std::min)(m_maxReconnectSeconds, reconnectSeconds * 2);
	}
	NotifyState(SessionState::Disconnected, "已关闭");
}

void CSession::MaintenanceLoop()
{
	std::chrono::steady_clock::time_point nextHeartbeat = std::chrono::steady_clock::now();
	while (!m_stopping.load())
	{
		std::unique_lock<std::mutex> lck(m_mtx_loops);
		m_cv_loops.wait_for(lck, std::chrono::milliseconds(250), [this]()
							{ return m_stopping.load(); });
		lck.unlock();
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
			for (auto mIter = m_reqs_sendout.begin(); m_reqs_sendout.end() != mIter;)
			{
				if (mIter->second.m_deadline <= now)
				{
					expired.push_back({ mIter->first, mIter->second.m_cmd, {}, "请求超时" });
					mIter = m_reqs_sendout.erase(mIter);
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

int CSession::OnNetEvent(const net::CNetEvent& ev)
{
	if (net::em_event::connected == ev.m_event)
	{
		m_state.store(SessionState::Connected);
		NotifyState(SessionState::Connected, "已连接");
		SendAuthentication();
		return 1;
	}
	if ((net::em_event::request == ev.m_event) && (nullptr != ev.m_request))
	{
		HandleResponse(*ev.m_request);
		return 1;
	}

	bool bAuthed = IsAuthenticated();
	m_state.store(SessionState::Disconnected);
	NotifyState(SessionState::Disconnected, "连接已断开");
	NotifyError(0 == ev.m_error ? "服务器主动关闭连接" : "网络错误：" + std::to_string(ev.m_error));
	if (bAuthed)
	{
		NotifyState(SessionState::Disconnected, "认证连接已断开");
	}
	return 1;
}

void CSession::HandleResponse(const CRequest& response)
{
	_TyRequestId id = response.GetId();
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		if (0 != id)
		{
			m_reqs_sendout.erase(id);
		}
	}

	std::string strCmd = response.GetCmd();

	std::optional<std::pair<int, std::string>> errorInfo = response.GetErrorInfo();
	std::string strError;
	if (errorInfo.has_value())
	{
		strError = errorInfo->second;
	}

	CRequest::Type t = response.GetType();
	if ((CRequest::Type::HEARTBEAT == t) && ("heartbeat" == strCmd))
	{
		return;
	}

	if (((CRequest::Type::QUERY_AUTH == t) && ("auth" == strCmd)) || ((CRequest::Type::UPDATE_AUTH == t) && ("register" == strCmd)))
	{
		AuthOperation op = CRequest::Type::QUERY_AUTH == t ? AuthOperation::Login : AuthOperation::Register;
		std::optional<CAuthParam> authParam;
		{
			std::lock_guard<std::mutex> lock(m_mtx_auth);
			if (m_authContext.has_value())
			{
				authParam = m_authContext->m_param;
				op = authParam->m_operation;
			}
		}
		if (errorInfo.has_value())
		{
			m_state.store(SessionState::Connected);
			if (authParam.has_value())
			{
				NotifyAuthentication({ op, AuthState::Failed, AuthError::AuthenticationFailed, false, strError });
			}
			else
			{
				{
					std::lock_guard<std::mutex> lock(m_mtx_auth);
					m_loginInfo.reset();
				}
				NotifyError("重新认证失败：" + strError);
			}
			return;
		}

		if (AuthOperation::Login == op)
		{
			if (authParam.has_value())
			{
				CLoginInfo info;
				info.m_strAccount = authParam->m_strAccount;
				info.m_strToken = response.GetReturnData("token");
				info.m_host = authParam->m_host;
				std::lock_guard<std::mutex> lock(m_mtx_auth);
				m_loginInfo = std::move(info);
			}
			m_state.store(SessionState::Ready);
			NotifyState(SessionState::Ready, "已认证");
			RestoreSubscriptions();
		}
		else
		{
			m_state.store(SessionState::Connected);
		}
		if (authParam.has_value())
		{
			NotifyAuthentication({ op, AuthState::Success, AuthError::None, true, AuthOperation::Login == op ? "登录成功" : "注册成功" });
		}

		return;
	}

	request::RequestParameters ret = response.GetReturnData();
	SessionResponse sessionResponse{ id, std::move(strCmd), std::move(ret), std::move(strError) };
	sessionResponse.m_message.CopyFrom(response.GetData());
	NotifyResponse(std::move(sessionResponse));
}

void CSession::SendAuthentication()
{
	std::optional<CAuthParam> auth_param;
	std::optional<CLoginInfo> login_info;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		if (m_authContext.has_value())
		{
			auth_param = m_authContext->m_param;
		}
		else if (m_loginInfo.has_value())
		{
			login_info = m_loginInfo;
		}
	}

	if (auth_param.has_value())
	{
		AuthOperation op = auth_param->m_operation;
		m_state.store(SessionState::Authenticating);
		NotifyAuthentication({ op, AuthState::Authenticating, AuthError::None, false, AuthOperation::Login == op ? "正在认证" : "正在注册" });

		request::AuthAction action = AuthOperation::Login == op ? request::AuthAction::Login : request::AuthAction::Register;
		if (!SendRequest(request::Auth(action, auth_param->m_strAccount, auth_param->m_strPassword)))
		{
			NotifyAuthentication({ op, AuthState::Failed, AuthError::NetworkError, false, "认证请求发送失败" });
		}
		return;
	}

	if (!login_info.has_value())
	{
		return;
	}
	if (!login_info->Valid())
	{
		NotifyError("登录信息中没有有效 token，无法重新认证");
		return;
	}

	m_state.store(SessionState::Authenticating);
	NotifyState(SessionState::Authenticating, "正在重新认证");
	if (!SendRequest(request::Auth(login_info->m_strToken)))
	{
		m_state.store(SessionState::Connected);
		NotifyError("重新认证请求发送失败");
	}
}

bool CSession::SendRequest(const CRequest& req)
{
	CRequest::Type t = req.GetType();
	bool bAuthRequest = (CRequest::Type::QUERY_AUTH == t) || (CRequest::Type::UPDATE_AUTH == t);
	if (!bAuthRequest && !IsAuthenticated())
	{
		return false;
	}

	_TyRequestId id = req.GetId();
	std::string strCmd = req.GetCmd();
	if (!bAuthRequest)
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		m_reqs_sendout.emplace(id, PendingRequest{ strCmd, std::chrono::steady_clock::now() + std::chrono::seconds(m_timeoutSeconds) });
	}
	bool bRet = false;
	{
		std::lock_guard<std::mutex> lock(m_mtx_client);
		bRet = (nullptr != m_client) && m_client->SendRequest(req);
	}
	if (!bRet)
	{
		if (!bAuthRequest)
		{
			std::lock_guard<std::mutex> lock(m_mtx_pending);
			if (0 != id)
			{
				m_reqs_sendout.erase(id);
			}
		}
	}
	return bRet;
}

void CSession::RestoreSubscriptions()
{
	decltype(m_subscriptions) sub;
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		sub = m_subscriptions;
	}
	for (const auto& [k, v] : sub)
	{
		SendRequest(request::Subscription(v.m_param));
	}
}

void CSession::FailPending(const std::string& strReson)
{
	std::vector<SessionResponse> failed;
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		failed.reserve(m_reqs_sendout.size());
		for (const auto& [id, pending] : m_reqs_sendout)
		{
			failed.push_back({ id, pending.m_cmd, {}, strReson });
		}
		m_reqs_sendout.clear();
	}
	for (auto& response : failed)
	{
		NotifyResponse(std::move(response));
	}
}

void CSession::NotifyAuthentication(AuthEvent ev)
{
	AuthCallback cb;
	{
		std::lock_guard<std::mutex> lock(m_mtx_auth);
		if (!m_authContext.has_value())
		{
			return;
		}
		if ((AuthState::Success == ev.m_state) || (AuthState::Failed == ev.m_state) || (AuthState::Cancelled == ev.m_state))
		{
			cb = std::move(m_authContext->m_callback);
			m_authContext.reset();
		}
		else
		{
			cb = m_authContext->m_callback;
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
	std::vector<ResponseCallback> callbacks;
	{
		std::lock_guard<std::mutex> lock(m_mtx_callbacks);
		callbacks = m_responseCallbacks;
	}
	for (const ResponseCallback& cb : callbacks)
	{
		if (nullptr != cb)
		{
			cb(response);
		}
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
