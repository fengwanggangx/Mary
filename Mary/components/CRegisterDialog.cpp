#include "CRegisterDialog.h"

#include "../configuration/CHostMgr.h"

#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>

#include <optional>
#include <utility>

CRegisterDialog::CRegisterDialog(QWidget* parent) : QDialog(parent)
{
	m_ui.setupUi(this);
	connect(m_ui.registerButton, &QPushButton::clicked, this, &CRegisterDialog::Register);
	connect(m_ui.cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	m_callbackId = CLoginService::InstanceRef().Subscribe([this](const AuthEvent& event)
	{
		if (AuthOperation::Register != event.m_operation)
		{
			return;
		}
		QMetaObject::invokeMethod(this, [this, event]()
		{
			if (AuthState::Success == event.m_state)
			{
				QMessageBox::information(this, "注册成功", QString::fromStdString(event.m_message));
				accept();
			}
			else if (AuthState::Failed == event.m_state)
			{
				m_ui.registerButton->setEnabled(true);
				QMessageBox::warning(this, "注册失败", QString::fromStdString(event.m_message));
			}
		}, Qt::QueuedConnection);
	});
}

CRegisterDialog::~CRegisterDialog()
{
	CLoginService::InstanceRef().Unsubscribe(m_callbackId);
}

void CRegisterDialog::SetAccount(const QString& account)
{
	m_ui.accountEdit->setText(account);
}

QString CRegisterDialog::Account() const
{
	return m_ui.accountEdit->text().trimmed();
}

void CRegisterDialog::Register()
{
	QString account = Account();
	QString password = m_ui.passwordEdit->text();
	if (account.isEmpty() || password.isEmpty())
	{
		QMessageBox::warning(this, "注册失败", "账号和密码不能为空。");
		return;
	}
	if (password != m_ui.confirmPasswordEdit->text())
	{
		QMessageBox::warning(this, "注册失败", "两次输入的密码不一致。");
		return;
	}

	std::optional<CHostInfo> site = CHostMgr::InstanceRef().GetActiveHost();
	if (!site.has_value())
	{
		QMessageBox::warning(this, "注册失败", "没有可用的服务器配置。");
		return;
	}

	m_ui.registerButton->setEnabled(false);
	CRegisterParam param;
	param.m_strAccount = account.toStdString();
	param.m_strPassword = password.toStdString();
	param.m_host = std::move(*site);
	CLoginService::InstanceRef().Register(param);
}
