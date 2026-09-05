#include "LoginService.h"

namespace
{
	class LocalLoginClient final : public ILoginClient
	{
	public:
		void Login(const LoginRequest& request, Callback callback) override
		{
			LoginEvent event;
			event.success = (("admin" == request.account) || ("fengwanggang" == request.account)) && ("123456" == request.password);
			event.message = event.success ? "登录成功" : "账号或密码错误，请重新输入";
			if (callback) 
			{ 
				callback(event); 
			}
		}
	};
}

LoginService::LoginService(std::unique_ptr<ILoginClient> client) : m_client(std::move(client)) 
{

}

CallbackId LoginService::Subscribe(std::function<void(const LoginEvent&)> callback) 
{ 
	return m_events.Subscribe(std::move(callback)); 
}

void LoginService::Unsubscribe(CallbackId id) 
{ 
	m_events.Unsubscribe(id); 
}

void LoginService::Login(const LoginRequest& request)
{
	if (nullptr == m_client) 
	{ 
		return; 
	}
	m_client->Login(request, [this](const LoginEvent& event) { m_events.Notify(event); });
}

std::unique_ptr<ILoginClient> MakeLocalLoginClient() 
{ 
	return std::make_unique<LocalLoginClient>(); 
}
