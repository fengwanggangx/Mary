#include "components/CLoginWindow.h"
#include "components/CMainWindow.h"
#include "log/defines_log.h"
#include "system/CBootLoader.h"
#include "system/CSession.h"

#include <QtWidgets/QApplication>
#include <QIcon>

#include <iostream>

int main(int argc, char* argv[])
{
	QApplication application(argc, argv);
	application.setWindowIcon(QIcon(":/branding/mary-app-icon.png"));
	CBootLoader boot;
	if (!boot.Initialize())
	{
		std::cerr << boot.GetLastError() << '\n';
		return boot.GetErrorCode();
	}

	if (!boot.Run())
	{
		std::cerr << boot.GetLastError() << '\n';
		return boot.GetErrorCode();
	}

	LoginWindow loginWindow;
	if (QDialog::Accepted != loginWindow.exec())
	{
		CSession::InstanceRef().Stop();
		boot.Finalize();
		CLogger::InstancePtr()->ShutDown();
		return 0;
	}

	CMainWindow mainWindow;
	mainWindow.show();
	int result = application.exec();

	CSession::InstanceRef().Stop();
	boot.Finalize();
	CLogger::InstancePtr()->ShutDown();
	return result;
}
