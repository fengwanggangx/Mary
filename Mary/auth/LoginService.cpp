#include "LoginService.h"

_TyCallbackId LoginService::Subscribe(std::function<void(const LoginEvent&)> callback) 
{ 
	return m_events.Subscribe(std::move(callback)); 
}

void LoginService::Unsubscribe(_TyCallbackId id) 
{ 
	m_events.Unsubscribe(id); 
}

void LoginService::Login(const CLoginParam& param)
{
	LoginEvent event;
	event.success = (("admin" == param.account) || ("fengwanggang" == param.account)) && ("123456" == param.password);
	event.message = event.success ? "登录成功" : "账号或密码错误，请重新输入";
	m_events.Notify(event);
}
