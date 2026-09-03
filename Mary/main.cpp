#include "components/CLoginWindow.h"
#include "components/CMainWindow.h"
#include "log/defines_log.h"

#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
	QApplication application(argc, argv);
	LoginWindow loginWindow;
	int result = 0;
	if (QDialog::Accepted == loginWindow.exec())
	{
		CMainWindow mainWindow;
		mainWindow.show();
		result = application.exec();
	}
	CLogger::InstancePtr()->ShutDown();
	return result;
}
