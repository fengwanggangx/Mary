#include "CLoginWindow.h"
#include "ui_CLoginWindow.h"
#include "CUIStyle.h"
#include "CServerSettingsDialog.h"
#include "CRegisterDialog.h"
#include "../cryption/Cryption.h"
#include "../system/CHostMgr.h"
#include <QComboBox>
#include <QDateTime>
#include <QLineEdit>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QPainter>
#include <QPointer>
#include <QSettings>
#include <QStyledItemDelegate>
#include <algorithm>
#include <functional>

namespace
{
	class CAccountItemDelegate final : public QStyledItemDelegate
	{
	  public:
		using _TyDeleteCallback = std::function<void(int)>;

		CAccountItemDelegate(_TyDeleteCallback&& callback, QObject* pParent) : QStyledItemDelegate(pParent), m_callback(std::move(callback))
		{
		}

		void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
		{
			QStyleOptionViewItem itemOption(option);
			itemOption.rect.adjust(0, 0, -24, 0);
			QStyledItemDelegate::paint(pPainter, itemOption, index);
			pPainter->save();
			pPainter->setPen(option.palette.color(QPalette::Text));
			pPainter->drawText(DeleteRect(option.rect), Qt::AlignCenter, QString::fromUtf8("×"));
			pPainter->restore();
		}

		bool editorEvent(QEvent* pEvent, QAbstractItemModel* pModel, const QStyleOptionViewItem& option, const QModelIndex& index) override
		{
			if ((QEvent::MouseButtonRelease == pEvent->type()) && DeleteRect(option.rect).contains(static_cast<QMouseEvent*>(pEvent)->position().toPoint()))
			{
				m_callback(index.row());
				return true;
			}
			return QStyledItemDelegate::editorEvent(pEvent, pModel, option, index);
		}

	  private:
		QRect DeleteRect(const QRect& rect) const
		{
			return QRect(rect.right() - 23, rect.top(), 24, rect.height());
		}

	  private:
		_TyDeleteCallback m_callback;
	};
} // namespace

LoginWindow::LoginWindow(QWidget* pParent) : QDialog(pParent), m_ui(std::make_unique<Ui::LoginWindowClass>())
{
	m_ui->setupUi(this);
	UIStyle::Apply(*this, ":/styles/login.qss");
	setWindowFlag(Qt::FramelessWindowHint);
	ConnectSlots();
	m_ui->checkBox_remember->setChecked(QSettings("Mary", "Mary").value("Login/RememberPassword", true).toBool());
	m_ui->comboBox_account->setItemDelegate(new CAccountItemDelegate([this](int nIndex)
																	 {
																		 DeleteAccount(nIndex);
																	 },
																	 m_ui->comboBox_account));
	LoadAccounts();
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
																									 safeThis->SavePendingAccount();
																									 safeThis->ClearPendingCredentials();
																									 safeThis->accept();
																								 }
																								 else if (AuthState::Failed == event.m_state)
																								 {
																									 safeThis->ClearPendingCredentials();
																									 safeThis->m_ui->pushButton_login->setEnabled(true);
																									 QMessageBox::information(safeThis.data(), "提示", QString::fromStdString(event.m_message));
																								 }
																								 else if (AuthState::Cancelled == event.m_state)
																								 {
																									 safeThis->ClearPendingCredentials();
																									 safeThis->m_ui->pushButton_login->setEnabled(true);
																								 }
																							 },
																							 Qt::QueuedConnection);
															   });
}

LoginWindow::~LoginWindow()
{
	ClearPendingCredentials();
	CLoginService::InstanceRef().Unsubscribe(m_loginCallbackId);
}

void LoginWindow::ConnectSlots()
{
	QObject::connect(m_ui->pushButton_login, &QPushButton::clicked, this, &LoginWindow::OnLoginBtnClicked);
	QObject::connect(m_ui->pushButton_register, &QPushButton::clicked, this, &LoginWindow::OnRegisterBtnClicked);
	QObject::connect(m_ui->pushButton_close, &QPushButton::clicked, this, &LoginWindow::OnCloseBtnClicked);
	QObject::connect(m_ui->pushButton_settings, &QPushButton::clicked, this, &LoginWindow::OnSettingsBtnClicked);
	QObject::connect(m_ui->comboBox_account, &QComboBox::currentIndexChanged, this, &LoginWindow::OnAccountChanged);
	QObject::connect(m_ui->comboBox_account->lineEdit(), &QLineEdit::textEdited, this, &LoginWindow::OnAccountEdited);
	QObject::connect(m_ui->checkBox_remember, &QCheckBox::toggled, this, [](bool bChecked)
					 {
						 QSettings("Mary", "Mary").setValue("Login/RememberPassword", bChecked);
					 });
}

void LoginWindow::mousePressEvent(QMouseEvent* event)
{
	if (Qt::LeftButton == event->button())
	{
		QWidget* child = childAt(event->pos());
		if ((nullptr == child) || (this == child) || ((nullptr == qobject_cast<QPushButton*>(child)) && (nullptr == qobject_cast<QLineEdit*>(child)) && (nullptr == qobject_cast<QComboBox*>(child))))
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

bool bTest = true;
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
	param.m_strAccount = m_ui->comboBox_account->currentText().toStdString();
	param.m_strPassword = m_ui->lineEdit_passwd->text().toStdString();
	param.m_host = std::move(*site);
	m_strPendingAccount = param.m_strAccount;
	m_strPendingPassword = param.m_strPassword;
	CLoginService::InstanceRef().Authenticate(param);
}

void LoginWindow::OnRegisterBtnClicked()
{
	CRegisterDialog dialog(this);
	dialog.SetAccount(m_ui->comboBox_account->currentText());
	if (QDialog::Accepted == dialog.exec())
	{
		m_ui->comboBox_account->setCurrentText(dialog.Account());
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

void LoginWindow::LoadAccounts()
{
	m_accounts = m_accountRepository.LoadAccounts();
	m_ui->lineEdit_passwd->clear();
	m_ui->comboBox_account->blockSignals(true);
	m_ui->comboBox_account->clear();
	for (const auto& account : m_accounts)
	{
		m_ui->comboBox_account->addItem(QString::fromStdString(account.m_strAccount));
	}
	m_ui->comboBox_account->blockSignals(false);
	if (!m_accounts.empty())
	{
		m_ui->comboBox_account->setCurrentIndex(0);
		FillPassword(0);
	}
}

void LoginWindow::FillPassword(int nIndex)
{
	m_ui->lineEdit_passwd->clear();
	if ((0 > nIndex) || (static_cast<std::size_t>(nIndex) >= m_accounts.size()))
	{
		return;
	}
	std::vector<std::uint8_t> password;
	std::string strError;
	if (crypto::CCryptor::InstanceRef().UnprotectCurrentUser(m_accounts[static_cast<std::size_t>(nIndex)].m_password, password, strError))
	{
		m_ui->lineEdit_passwd->setText(QString::fromUtf8(reinterpret_cast<const char*>(password.data()), static_cast<qsizetype>(password.size())));
	}
}

void LoginWindow::OnAccountChanged(int nIndex)
{
	FillPassword(nIndex);
}

void LoginWindow::OnAccountEdited(const QString& strAccount)
{
	int nIndex = m_ui->comboBox_account->findText(strAccount, Qt::MatchExactly);
	if (0 > nIndex)
	{
		m_ui->lineEdit_passwd->clear();
	}
}

void LoginWindow::SavePendingAccount()
{
	if (!m_ui->checkBox_remember->isChecked() || m_strPendingAccount.empty() || m_strPendingPassword.empty())
	{
		return;
	}
	std::vector<std::uint8_t> data(m_strPendingPassword.begin(), m_strPendingPassword.end());
	std::vector<std::uint8_t> protectedData;
	std::string strError;
	bool bProtected = crypto::CCryptor::InstanceRef().ProtectCurrentUser(data, protectedData, strError);
	std::fill(data.begin(), data.end(), 0);
	if (!bProtected)
	{
		return;
	}
	std::int64_t nTimestamp = QDateTime::currentMSecsSinceEpoch();
	if (m_accountRepository.SaveAccount(m_strPendingAccount, protectedData, nTimestamp))
	{
		LoadAccounts();
	}
}

void LoginWindow::DeleteAccount(int nIndex)
{
	if ((0 > nIndex) || (static_cast<std::size_t>(nIndex) >= m_accounts.size()))
	{
		return;
	}
	QString strAccount = QString::fromStdString(m_accounts[static_cast<std::size_t>(nIndex)].m_strAccount);
	if (QMessageBox::Yes != QMessageBox::question(this, "删除账号", QString("确定删除已保存账号“%1”吗？").arg(strAccount), QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
	{
		return;
	}
	if (m_accountRepository.DeleteAccount(strAccount.toStdString()))
	{
		LoadAccounts();
	}
}

void LoginWindow::ClearPendingCredentials()
{
	std::fill(m_strPendingPassword.begin(), m_strPendingPassword.end(), '\0');
	m_strPendingPassword.clear();
	m_strPendingAccount.clear();
}
