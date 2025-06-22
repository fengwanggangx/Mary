#ifndef __NET_COMMON_H__
#define __NET_COMMON_H__


namespace net
{
	bool IsThreadEnable();
	void EnvInitialize();
	bool FmtAddress(struct sockaddr_in& addr, int nPort, const std::string& strAddr = "");
	std::string ParseSockAddr(std::string& strAddr, int& nPort, const struct sockaddr& addr);
	bool CheckSockAddress(struct sockaddr* pAddr, int nLength);
	bool SockAddrSafeCopy(struct sockaddr& dst, const struct sockaddr& src);
}


#endif
