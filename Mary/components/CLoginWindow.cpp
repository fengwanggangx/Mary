#include "CLoginWindow.h"
#include "CServerSettingsDialog.h"
#include "../configuration/CServerSettings.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QMetaObject>

LoginWindow::LoginWindow(QWidget *parent) : QDialog(parent) , ui(new Ui::LoginWindowClass())
{
	ui->setupUi(this);
	setWindowFlag(Qt::FramelessWindowHint);
	ConnectSlots();	
	m_loginService = std::make_unique<LoginService>();
	m_loginCallbackId = m_loginService->Subscribe([this](const AuthEvent& event)
	{
		QMetaObject::invokeMethod(this, [this, event]()
		{
			if (AuthState::Success == event.state)
			{
				accept();
			}
			else if (AuthState::Failed == event.state)
			{
				ui->pushButton_login->setEnabled(true);
				QMessageBox::information(this, "Tips", QString::fromStdString(event.message));
			}
		}, Qt::QueuedConnection);
	});
}

LoginWindow::~LoginWindow()
{
	if (nullptr != m_loginService)
	{
		m_loginService->Unsubscribe(m_loginCallbackId);
	}
	delete ui;
}

void LoginWindow::ConnectSlots()
{
	QObject::connect(ui->pushButton_login, &QPushButton::clicked, this, &LoginWindow::OnLoginBtnClicked);
	QObject::connect(ui->pushButton_close, &QPushButton::clicked, this, &LoginWindow::OnCloseBtnClicked);
	QObject::connect(ui->pushButton_settings, &QPushButton::clicked, this, &LoginWindow::OnSettingsBtnClicked);
}

void LoginWindow::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) 
	{
		QWidget* child = childAt(event->pos());
		if ((nullptr == child) || (child == this) || ((qobject_cast<QPushButton*>(child) == nullptr)  && (qobject_cast<QLineEdit*>(child) == nullptr)))
		{
			m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
			m_isDragging = true;
			event->accept();
		}

	}
}
void LoginWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isDragging && (event->buttons() & Qt::LeftButton)) 
	{
		move(event->globalPosition().toPoint() - m_dragPosition);
		event->accept();
	}
	QDialog::mouseMoveEvent(event);
}

void LoginWindow::mouseReleaseEvent(QMouseEvent* event)
{
	m_isDragging = false;
	QDialog::mouseReleaseEvent(event);
}

void LoginWindow::OnLoginBtnClicked()
{
	const std::optional<configuration::CHostInfo> site = configuration::CHostMgr::InstanceRef().GetActiveHost();
	if (!site.has_value())
	{
		QMessageBox::warning(this, "登录失败", "没有可用的服务器配置。");
		return;
	}

	ui->pushButton_login->setEnabled(false);

	CLoginParam param;
	param.account = ui->lineEdit_account->text().toStdString();
	param.password = ui->lineEdit_passwd->text().toStdString();
	param.site.id = site->m_strKey;
	param.site.name = site->m_strName;
	param.site.host = site->m_strHost;
	param.site.port = static_cast<int>(site->m_nPort);
	m_loginService->Login(param);
}

void LoginWindow::OnCloseBtnClicked()
{
	reject();
	//QApplication::quit();
}

void LoginWindow::OnSettingsBtnClicked()
{
	CServerSettingsDialog dialog(this);
	dialog.exec();
}
