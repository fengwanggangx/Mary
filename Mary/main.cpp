#include "components/CLoginWindow.h"
#include "components/CMainWindow.h"
#include <QtWidgets/QApplication>
#include <event2/event.h>
#include <sqlite3.h>
#include "./database/CDBEngine.h"
#include "./log/Defines.h"
#include "./common/datetime.h"
#include "./network/CNetClient.h"
#include "./network/CNetServer.h"
#include "./network/common.h"
#include "./cryption/Cryption.h"
#include "./cfg/CINIHandler.h"
#include "./threadpool/CThreadPool.h"
#include "./request/request.h"


net::CNetServer* pServer = nullptr;
net::CNetClient* pClient = nullptr;

#define ThreadPoolPtr CThreadPool::InstancePtr(2, 3)

bool bServer = false;

void InitializeFramework()
{
	net::EnvInitialize();

	if (bServer)
	{
		if (nullptr == pServer)
		{
			pServer = new net::CNetServer(9877);
		}
		ThreadPoolPtr->PushTask(task_priority::em_high, 0, [](net::CNetServer* p) {
			pServer->Initialize();
			p->Start(false);
			}, 
			pServer);
	}
	else
	{
		if (nullptr == pClient)
		{
			pClient = new net::CNetClient("127.0.0.1", 9877);
		}
		ThreadPoolPtr->PushTask(task_priority::em_high, 0, [](net::CNetClient* p) {
			p->Initialize();
			p->Start(true);
			}, 
			pClient);
	}

}

void test()
{
	CRequest* req = new CRequest;
	req->SetType(CRequest::Type::QUERY_AUTH);
	std::string s = "create_user";
	req->SetCmd(s);

	// 添加参数
	req->SetExtraData("username", "john_doe");
	req->SetExtraData("email", "john@example.com");

	std::string data;
	if (req->Serialize(&data))
	{
	}
}

int main(int argc, char *argv[])
{
	InitializeFramework();
    QApplication a(argc, argv);
    LoginWindow loginWnd;
	//test();

    if (loginWnd.exec() == QDialog::Accepted)
    {
		CMainWindow w;
		w.show();
		int nRet = a.exec();
		CLogger::InstancePtr()->ShutDown();
		return nRet;
    }
	CLogger::InstancePtr()->ShutDown();
    return 0;
}
