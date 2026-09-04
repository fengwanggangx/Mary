#include "components/CLoginWindow.h"
#include "components/CMainWindow.h"
#include "log/defines_log.h"
#include "system/CBootLoader.h"

#include <QtWidgets/QApplication>

#include <iostream>
#include <thread>

int main(int argc, char* argv[])
{
	QApplication application(argc, argv);
	CBootLoader boot;
	if (!boot.Initialize())
	{
		std::cerr << boot.GetLastError() << '\n';
		return boot.GetErrorCode();
	}

	int bootResult = 0;
	std::thread bootThread([&boot, &bootResult]()
	{
		if (!boot.Run())
		{
			bootResult = boot.GetErrorCode();
			std::cerr << boot.GetLastError() << '\n';
		}
	});

	LoginWindow loginWindow;
	int result = 0;
	if (QDialog::Accepted == loginWindow.exec())
	{
		CMainWindow mainWindow;
		mainWindow.show();
		result = application.exec();
	}

	boot.Stop();
	if (bootThread.joinable())
	{
		bootThread.join();
	}
	boot.Finalize();
	CLogger::InstancePtr()->ShutDown();
	return 0 == bootResult ? result : bootResult;
}
