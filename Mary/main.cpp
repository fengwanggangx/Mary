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
#include "./ini/CINIHandler.h"
#include "./thread/CThreadPool.h"
#include "./request/request.h"
#include "./network/CNetDistributor.h"
#include "./xml/XmlDocument.h"
#include "./xml/XmlAttribute.h"


net::CNetServer* pServer = nullptr;
net::CNetClient* pClient = nullptr;

#define ThreadPoolPtr CThreadPool::InstancePtr(2, 3)

bool bServer = false;

void InitializeFramework()
{
	net::CNetDistributor<CRequest>::InstancePtr();
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
			pClient = new net::CNetClient("172.17.93.107", 9877);
		}
		ThreadPoolPtr->PushTask(task_priority::em_high, 0, [](net::CNetClient* p) {
			p->Initialize();
			p->Start(true);
			}, 
			pClient);
	}

}


int main(int argc, char *argv[])
{
	xml::XmlDocument doc;
	bool bRet = doc.LoadFromFile("E:/repos/Mary/x64/Debug/test.xml");
	xml::XmlNode root = doc.GetRoot();
	std::string xxx = root.GetName();
	auto nods = root.GetChildren("MethodInfo");
	for (auto& elem : nods) 
	{
		std::string xxx1 = elem.GetName();
		auto zz = elem.GetAttributes();
		for (auto& d : zz)
		{
			std::string x1 = d.GetName();
			std::string x2 = d.GetValue();
			int dd = 2;
		}
		int x = 1;
	}

	auto nods1 = root.GetChildren("MethodInfo3");
	for (auto& elem : nods1)
	{
		std::string xxx1 = elem.GetName();
		int x = 1;
	}

	int x = 1;
	//InitializeFramework();
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
