#include "CLoginWindow.h"
#include "ui_CLoginWindow.h"
#include "CUIStyle.h"
#include "CServerSettingsDialog.h"
#include "CRegisterDialog.h"
#include "../system/CHostMgr.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>

LoginWindow::LoginWindow(QWidget* pParent) : QDialog(pParent), m_ui(std::make_unique<Ui::LoginWindowClass>())
{
	m_ui->setupUi(this);
	UIStyle::Apply(*this, ":/styles/login.qss");
	setWindowFlag(Qt::FramelessWindowHint);
	ConnectSlots();
	QPointer<LoginWindow> safeThis(this);
	m_loginCallbackId = CLoginService::InstanceRef().Subscribe([safeThis](const AuthEvent& event)
	{
		if (safeThis.isNull())
		{
			return;
		}
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, event]()
		{
			if (safeThis.isNull() || (AuthOperation::Login != event.m_operation))
			{
				return;
			}
			if (AuthState::Success == event.m_state)
			{
				safeThis->accept();
			}
			else if (AuthState::Failed == event.m_state)
			{
				safeThis->m_ui->pushButton_login->setEnabled(true);
				QMessageBox::information(safeThis.data(), "提示", QString::fromStdString(event.m_message));
			}
		}, Qt::QueuedConnection);
	});
}

LoginWindow::~LoginWindow()
{
	CLoginService::InstanceRef().Unsubscribe(m_loginCallbackId);
}

void LoginWindow::ConnectSlots()
{
	QObject::connect(m_ui->pushButton_login, &QPushButton::clicked, this, &LoginWindow::OnLoginBtnClicked);
	QObject::connect(m_ui->pushButton_register, &QPushButton::clicked, this, &LoginWindow::OnRegisterBtnClicked);
	QObject::connect(m_ui->pushButton_close, &QPushButton::clicked, this, &LoginWindow::OnCloseBtnClicked);
	QObject::connect(m_ui->pushButton_settings, &QPushButton::clicked, this, &LoginWindow::OnSettingsBtnClicked);
}

void LoginWindow::mousePressEvent(QMouseEvent* event)
{
	if (Qt::LeftButton == event->button())
	{
		QWidget* child = childAt(event->pos());
		if ((nullptr == child) || (this == child) || ((nullptr == qobject_cast<QPushButton*>(child)) && (nullptr == qobject_cast<QLineEdit*>(child))))
		{
			m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
			m_isDragging = true;
			event->accept();
		}
	}
}
void LoginWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isDragging && (0 != (event->buttons() & Qt::LeftButton)))
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

bool bTest = false;
void LoginWindow::OnLoginBtnClicked()
{
	if (bTest)
	{
		accept();
		return;
	}
	std::optional<CHostInfo> site = CHostMgr::InstanceRef().GetActiveHost();
	if (!site.has_value())
	{
		QMessageBox::warning(this, "登录失败", "没有可用的服务器配置。");
		return;
	}

	m_ui->pushButton_login->setEnabled(false);

	CAuthParam param;
	param.m_operation = AuthOperation::Login;
	param.m_strAccount = m_ui->lineEdit_account->text().toStdString();
	param.m_strPassword = m_ui->lineEdit_passwd->text().toStdString();
	param.m_host = std::move(*site);
	CLoginService::InstanceRef().Authenticate(param);
}

void LoginWindow::OnRegisterBtnClicked()
{
	CRegisterDialog dialog(this);
	dialog.SetAccount(m_ui->lineEdit_account->text());
	if (QDialog::Accepted == dialog.exec())
	{
		m_ui->lineEdit_account->setText(dialog.Account());
		m_ui->lineEdit_passwd->clear();
		m_ui->lineEdit_passwd->setFocus();
	}
}

void LoginWindow::OnCloseBtnClicked()
{
	reject();
	// QApplication::quit();
}

void LoginWindow::OnSettingsBtnClicked()
{
	CServerSettingsDialog dialog(this);
	dialog.exec();
}
