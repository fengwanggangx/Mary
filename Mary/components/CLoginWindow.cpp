#include "CLoginWindow.h"
#include "CServerSettingsDialog.h"
#include "CRegisterDialog.h"
#include "../configuration/CHostMgr.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QMetaObject>

LoginWindow::LoginWindow(QWidget* parent) : QDialog(parent), ui(new Ui::LoginWindowClass())
{
	ui->setupUi(this);
	setWindowFlag(Qt::FramelessWindowHint);
	ConnectSlots();	
	m_loginCallbackId = CLoginService::InstanceRef().Subscribe([this](const AuthEvent& event)
	{
		QMetaObject::invokeMethod(this, [this, event]()
		{
			if (AuthOperation::Login != event.m_operation)
			{
				return;
			}
			if (AuthState::Success == event.m_state)
			{
				accept();
			}
			else if (AuthState::Failed == event.m_state)
			{
				ui->pushButton_login->setEnabled(true);
				QMessageBox::information(this, "提示", QString::fromStdString(event.m_message));
			}
		}, Qt::QueuedConnection);
	});
}

LoginWindow::~LoginWindow()
{
	CLoginService::InstanceRef().Unsubscribe(m_loginCallbackId);
	delete ui;
}

void LoginWindow::ConnectSlots()
{
	QObject::connect(ui->pushButton_login, &QPushButton::clicked, this, &LoginWindow::OnLoginBtnClicked);
	QObject::connect(ui->pushButton_register, &QPushButton::clicked, this, &LoginWindow::OnRegisterBtnClicked);
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
	std::optional<CHostInfo> site = CHostMgr::InstanceRef().GetActiveHost();
	if (!site.has_value())
	{
		QMessageBox::warning(this, "登录失败", "没有可用的服务器配置。");
		return;
	}

	ui->pushButton_login->setEnabled(false);

	CLoginParam param;
	param.m_strAccount = ui->lineEdit_account->text().toStdString();
	param.m_strPassword = ui->lineEdit_passwd->text().toStdString();
	param.m_host = std::move(*site);
	CLoginService::InstanceRef().Login(param);
}

void LoginWindow::OnRegisterBtnClicked()
{
	CRegisterDialog dialog(this);
	dialog.SetAccount(ui->lineEdit_account->text());
	if (QDialog::Accepted == dialog.exec())
	{
		ui->lineEdit_account->setText(dialog.Account());
		ui->lineEdit_passwd->clear();
		ui->lineEdit_passwd->setFocus();
	}
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
