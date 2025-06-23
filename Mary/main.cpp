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
#include "./protobuf/addressbook.pb.h"
#include "./cfg/CINIHandler.h"
#include "./threadpool/CThreadPool.h"


net::CNetServer* pServer = nullptr;
void server()
{
	if (nullptr == pServer)
	{
		pServer = new net::CNetServer(9877);
		//pServer->Initialize();
		pServer->Start();
	}
}

void client()
{
	if (nullptr != pServer)
	{
		pServer->Initialize();
	}
}

#define ThreadPoolPtr CThreadPool::InstancePtr(2, 3);
int main(int argc, char *argv[])
{
	ThreadPoolPtr;
	int x =  ini::CINIHandler::InstancePtr()->GetValue(ini::cfgs::system, "xxx", "ffff", 1);
	std::string x1 = ini::CINIHandler::InstancePtr()->GetValue(ini::cfgs::system, "xxx", "ffff", "");
	ini::CINIHandler::InstancePtr()->SetValue(ini::cfgs::system, "xxx", "ffff", "111");
	ini::CINIHandler::InstancePtr()->SetValue(ini::cfgs::system, "xxx", "ffff", 111);
	//tutorial::AddressBook address_book;
	net::EnvInitialize();
	std::thread t(server);
	t.detach();
    QApplication a(argc, argv);
    LoginWindow loginWnd;
    if (loginWnd.exec() == QDialog::Accepted)
    {
		std::thread t1(client);
		t1.detach();
		CMainWindow w;
		w.show();
		int nRet = a.exec();
		CLogger::InstancePtr()->ShutDown();
		return nRet;
    }
	CLogger::InstancePtr()->ShutDown();
    return 0;
}
