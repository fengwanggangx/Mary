#include "RequestCenter.h"
#include "../request/request.h"
#include <string>
#include "../network/CNetPool.h"

int Query(const std::unique_ptr<CRequest>& req)
{
	std::string s = req->GetCmd();
	std::string s1 = req->GetExtraData("retmsg");
	int64_t fd = req->GetFd();
	return 0;
}

int Update(const std::unique_ptr<CRequest>& req)
{
	std::string s = req->GetCmd();
	std::string s1 = req->GetExtraData("retmsg");
	int64_t fd = req->GetFd();
	return 0;
}

int Auth(const std::unique_ptr<CRequest>& req)
{
	std::string s = req->GetCmd();
	std::string s1 = req->GetExtraData("retmsg");
	int64_t fd = req->GetFd();
	net::CNetPool::InstancePtr()->SendReq2Client(fd, req);
	return 0;
}