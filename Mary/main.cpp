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
#include "./xml/XmlDocument.h"
#include "./xml/XmlAttribute.h"
#include "base/CDatable.h"
#include "./base/CDistributor.h"
#include "./lua/CLuaVM.h"
#include <memory>

net::CNetClient* pClient = nullptr;

bool bServer = false;

void InitializeFramework()
{
	net::EnvInitialize();
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

#include "./lua/CLuaParam.h"
int main(int argc, char *argv[])
{
	CLuaVM vm;
	bool bRet = vm.LoadScript("C:/Users/X/Desktop/main.lua");
	CLuaParam* p = new CLuaParam();
	vm.Execute(1, p);

	InitializeFramework();
	std::unique_ptr<CDistributor<true, std::vector<std::unique_ptr<CRequest>>, std::function<int(const CRequest&)>>> m_dispatcher;
    QApplication a(argc, argv);
    LoginWindow loginWnd;

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
