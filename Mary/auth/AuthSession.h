#pragma once

#include "CLoginService.h"
#include "../network/CTcpClient.h"
#include <functional>
#include <memory>

class AuthSession final
{
public:
	using Callback = std::function<void(const AuthEvent&)>;

	AuthSession() = default;
	~AuthSession();
	AuthSession(const AuthSession&) = delete;
	AuthSession& operator=(const AuthSession&) = delete;

	void Start(AuthOperation operation, const CLoginParam& param, Callback callback);
	void Cancel();
	bool IsRunning() const noexcept;

private:
	void OnNetworkEvent(const net::CNetEvent& event);
	void SendRequest();
	void Notify(AuthEvent event);

	std::unique_ptr<net::CTcpClient> m_client;
	AuthOperation m_operation{ AuthOperation::Login };
	CLoginParam m_param;
	Callback m_callback;
	bool m_running{false};
};
