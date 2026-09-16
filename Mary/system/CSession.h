#ifndef MARY_SERVER_CSESSION_H
#define MARY_SERVER_CSESSION_H

#include "../common/ISingleton.h"
#include "../request/request.pb.h"
#include "CLoginService.h"
#include "../request/RequestCenter.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace net
{
	class CTcpClient;
	struct CNetEvent;
} // namespace net

enum class SessionState
{
	Disconnected,
	Connecting,
	Connected,
	Authenticating,
	Ready,
	Reconnecting,
	Stopping
};

struct SessionResponse
{
	std::uint64_t m_id{ 0 };
	std::string m_cmd;
	request::RequestParameters m_result;
	std::string m_error;
	_TyReqData m_message;
};

class CSession final : public ISingleton<CSession>
{
	DECLARE_SINGLE_DFAULT(CSession)

  public:
	using AuthCallback = std::function<void(const AuthEvent&)>;
	using StateCallback = std::function<void(SessionState, const std::string&)>;
	using ResponseCallback = std::function<void(const SessionResponse&)>;
	using ErrorCallback = std::function<void(const std::string&)>;

	void Authenticate(const CAuthParam& param, AuthCallback&& cb);
	void CancelAuthentication();
	void Stop();
	bool SendRequest(const CRequest& request);
	void SetStateCallback(StateCallback&& cb);
	void RegisterStateHandler(StateCallback&& cb);
	void SetResponseCallback(ResponseCallback&& cb);
	void RegisterResponseHandler(ResponseCallback&& cb);
	void SetErrorCallback(ErrorCallback&& cb);
	bool IsAuthenticated() const noexcept;
	SessionState GetState() const noexcept;
	std::optional<CLoginInfo> GetLoginInfo() const;

  private:
	struct PendingRequest
	{
		std::string m_cmd;
		std::chrono::steady_clock::time_point m_deadline;
	};

	struct AuthContext
	{
		CAuthParam m_param;
		AuthCallback m_callback;
	};

	void StartConnection();
	void ConnectionLoop();
	void MaintenanceLoop();
	int OnNetEvent(const net::CNetEvent& ev);
	void HandleResponse(const CRequest& response);
	void SendAuthentication();
	void FailPending(const std::string& reason);
	void NotifyAuthentication(AuthEvent ev);
	void NotifyState(SessionState state, const std::string& message);
	void NotifyResponse(SessionResponse response);
	void NotifyError(const std::string& error);

  private:
	mutable std::mutex m_mtx_client;
	std::unique_ptr<net::CTcpClient> m_client;

  private:
	std::atomic_bool m_stopping{ false };
	std::atomic<SessionState> m_state{ SessionState::Disconnected };

	std::thread m_thread_conn;		// 连接、收包、断线重连
	std::thread m_thread_heartbeat; // 发送心跳、检查请求超时

	std::mutex m_mtx_loops;
	std::condition_variable m_cv_loops;

	mutable std::mutex m_mtx_auth;
	std::optional<AuthContext> m_authContext;
	std::optional<CLoginInfo> m_loginInfo;

	std::mutex m_mtx_pending;
	std::unordered_map<_TyRequestId, PendingRequest> m_reqs_sendout;
	std::mutex m_mtx_callbacks;
	StateCallback m_stateCallback;
	std::vector<StateCallback> m_stateHandlers;
	std::vector<ResponseCallback> m_responseCallbacks;
	ErrorCallback m_errorCallback;

	int m_heartbeatSeconds{ 15 };
	int m_timeoutSeconds{ 10 };
	int m_maxReconnectSeconds{ 30 };
};

#endif
