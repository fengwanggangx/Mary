#ifndef __CNETPOOL_H__
#define __CNETPOOL_H__


#include <unordered_map>
#include "../common/ISingleton.h"
#include "event2/bufferevent.h"
#include <memory>

class CRequest;
using _TyFd = evutil_socket_t;
namespace net
{	
	struct CNetInfo;
	class CNetPool final : public ISingleton<CNetPool>
	{
		DECLARE_SINGLE_DFAULT(CNetPool)

	public:
		bool CloseAConnection(_TyFd fd);
		struct bufferevent* RegisterConnect(_TyFd fd, struct event_base* pNet, struct sockaddr* pAddr, int nLength, bufferevent_data_cb readcb, bufferevent_data_cb writecb, bufferevent_event_cb eventcb, void* cbarg);
		struct bufferevent* RegisterAConnection(_TyFd fd, struct bufferevent* pEvent, struct sockaddr_storage* pAddr);
	
	public:

		[[deprecated("use SendReq2Client() instead")]]
		bool SendData2Client(_TyFd fd, const char* data, size_t nLength);
		bool SendReq2Client(_TyFd fd, const std::unique_ptr<CRequest>& req);
	private:
		bool CloseAConnection(CNetInfo& info);
		bool RegisterAConnection(_TyFd fd, struct bufferevent* pEvent, struct sockaddr* pAddr);

		
	private:
		std::unordered_map<_TyFd, CNetInfo*> m_pool;
	};
}
#endif
