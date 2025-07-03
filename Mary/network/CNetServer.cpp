#include "CNetServer.h"
#include <event2/buffer.h>  // 主要头文件
#include "CNetParser.h"
#include <iostream>
#include <event2/thread.h>
#include <event2/listener.h>
#include <chrono>
#include <thread>
#include "common.h"
#include "CNetPool.h"
#include "../request/request.h"

namespace net
{

	CNetServer::CNetServer(int nPort) : m_nPort(nPort)
	{
		m_buffer.reserve(4096);
	}

	void CNetServer::OnListenerError(struct evconnlistener* pListener)
	{

	}

	void CNetServer::OnConnAccept(struct evconnlistener* pListener, evutil_socket_t fd, struct sockaddr* pAddr, int nLength)
	{
		if (nullptr == GetNet())
		{
			return;
		}

		CNetPool::InstancePtr()->RegisterConnect(fd, GetNet(), pAddr, nLength, CNetServer::Read_Callback, nullptr, CNetServer::Event_Callback, this);
	}

	void CNetServer::OnRead(struct bufferevent* pEvent)
	{
		int n = CNetParser::BufferEventReader(pEvent, m_buffer);
		if (n > 0)
		{
// 			std::string str(m_buffer.data(), n);
// 			const char* response = "Server has received your message";
// 			bufferevent_write(pEvent, response, strlen(response));

			std::string strData(m_buffer.data(), n);
			CRequest req;
			if (req.Deserialize(strData))
			{
				std::string st1 = req.GetCmd();
				std::unordered_map<std::string, std::string> x = req.GetExtraData();
				int x1 = 1;
			}
		}
	}

	void CNetServer::OnEvent(struct bufferevent* pEvent, short events)
	{
		if (nullptr == pEvent)
		{
			return;
		}
		evutil_socket_t fd = bufferevent_getfd(pEvent);
		if (fd < 0)
		{
			bufferevent_free(pEvent);
			pEvent = nullptr;
		}
		//BEV_EVENT_EOF:onnection closed   BEV_EVENT_ERROR:some other error  BEV_EVENT_TIMEOUT:
		CNetPool::InstancePtr()->CloseAConnection(fd);
	}

	int CNetServer::Initialize()
	{
		if (nullptr == GetNet())
		{
			return -1;
		}
		struct sockaddr_in svr;
		bool bRet = FmtAddress(svr, m_nPort);
		struct evconnlistener* pListener = evconnlistener_new_bind(
			GetNet(), 
			CNetServer::ConnAccept_Callback,
			this,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
			-1,
			(struct sockaddr*)&svr, 
			sizeof(svr));

		//evconnlistener_set_error_cb(pListener, ListenerErrorCallback);
		return 0;
	}
}