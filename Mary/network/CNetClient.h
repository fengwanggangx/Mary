#ifndef __CNETCLIENT_H__
#define __CNETCLIENT_H__
#include <vector>
#include <string>
#include "CNet.h"
#include "CNetRouter.h"

namespace net
{
	class CNetClient final : public CNet, public CNetRouter<CNetClient>
	{
	public:
		explicit CNetClient(const std::string& strAddr, int nPort);
		~CNetClient() = default;

	public:
		void Send(const char* pData);
		void Recv(const char* pData);
		void NetInitialize();
	private:
		int Initialize();

	public:
		void OnRead(struct bufferevent* pEvent) override;
		void OnEvent(struct bufferevent* pEvent, short events) override;
		void OnConnected(bufferevent* pEvent) override;

	private:
		std::string m_strAddr;
		int	m_nPort{ -1 };

		std::vector<char> m_buffer;
	};
}
#endif
