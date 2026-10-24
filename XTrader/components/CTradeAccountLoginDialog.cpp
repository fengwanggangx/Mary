#include "CTradeAccountLoginDialog.h"
#include "CUIStyle.h"
#include "CSinglePixelBorder.h"
#include "ui_CTradeAccountLoginDialog.h"
#include <QAction>
#include <QAbstractItemView>
#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>
#include <QSettings>
#include <QIcon>
#include <QMouseEvent>
#include <QWindow>

CTradeAccountLoginDialog::CTradeAccountLoginDialog(QWidget* pParent) : QDialog(pParent), m_ui(std::make_unique<Ui::CTradeAccountLoginDialog>())
{
	setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
	m_ui->setupUi(this);
	setFixedSize(390, 308);
	m_ui->titleFrame->installEventFilter(this);
	m_ui->titleLabel->installEventFilter(this);
	connect(m_ui->closeButton, &QPushButton::clicked, this, &QDialog::reject);
	UIStyle::Apply(*this, ":/styles/trade-account-login.qss");
	// Only the device-pixel overlay draws borders; disable native frames.
	m_ui->accountEdit->setFrame(false);
	m_ui->passwordEdit->setFrame(false);
	for (QComboBox* pCombo : { m_ui->brokerCombo, m_ui->siteCombo })
	{
		pCombo->setFrame(false);
		pCombo->view()->setFrameShape(QFrame::NoFrame);
		QFrame* pPopup = qobject_cast<QFrame*>(pCombo->view()->parentWidget());
		if (nullptr != pPopup)
		{
			pPopup->setFrameShape(QFrame::NoFrame);
		}
		CSinglePixelBorder::Install(*pCombo->view());
	}
	for (QWidget* pControl : { static_cast<QWidget*>(this), static_cast<QWidget*>(m_ui->accountEdit), static_cast<QWidget*>(m_ui->passwordEdit), static_cast<QWidget*>(m_ui->brokerCombo), static_cast<QWidget*>(m_ui->siteCombo) })
	{
		CSinglePixelBorder::Install(*pControl);
	}
	CSinglePixelBorder::Install(*m_ui->titleFrame, 0.0, Qt::BottomEdge);
	CSinglePixelBorder::Install(*m_ui->divider, 0.0, Qt::TopEdge);
	CSinglePixelBorder::Install(*m_ui->loginButton, 5.0);
	CSinglePixelBorder::Install(*m_ui->cancelButton, 5.0);
	QSettings settings(QCoreApplication::applicationDirPath() + "/ini/trade-accounts.ini", QSettings::IniFormat);
	settings.beginGroup("brokers");
	for (const auto& strBroker : settings.childGroups())
	{
		settings.beginGroup(strBroker);
		m_ui->brokerCombo->addItem(settings.value("name", strBroker).toString(), strBroker);
		settings.endGroup();
	}
	settings.endGroup();
	if (0 == m_ui->brokerCombo->count())
	{
		m_ui->brokerCombo->addItem("XEngine 模拟服务", "wind_simulated");
	}
	connect(m_ui->brokerCombo, &QComboBox::currentIndexChanged, this, [this]()
	{
		m_ui->siteCombo->clear();
		QSettings settings(QCoreApplication::applicationDirPath() + "/ini/trade-accounts.ini", QSettings::IniFormat);
		settings.beginGroup("brokers/" + m_ui->brokerCombo->currentData().toString() + "/sites");
		for (const auto& strSite : settings.childKeys())
		{
			m_ui->siteCombo->addItem(settings.value(strSite).toString(), strSite);
		}
		if ((0 == m_ui->siteCombo->count()) && ("wind_simulated" == m_ui->brokerCombo->currentData().toString()))
		{
			m_ui->siteCombo->addItem("默认站点", "default");
		}
	});
	m_ui->brokerCombo->setCurrentIndex(-1);
	m_ui->brokerCombo->setCurrentIndex(0);
	QAction* pReveal = m_ui->passwordEdit->addAction(QIcon(":/trade/password-eye.svg"), QLineEdit::TrailingPosition);
	pReveal->setCheckable(true);
	pReveal->setText("显示密码");
	pReveal->setToolTip("显示密码");
	connect(pReveal, &QAction::toggled, this, [this, pReveal](bool bVisible)
	{
		m_ui->passwordEdit->setEchoMode(bVisible ? QLineEdit::Normal : QLineEdit::Password);
		pReveal->setText(bVisible ? "隐藏密码" : "显示密码");
		pReveal->setToolTip(pReveal->text());
	});
	connect(m_ui->loginButton, &QPushButton::clicked, this, &CTradeAccountLoginDialog::Login);
	connect(m_ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(m_ui->passwordEdit, &QLineEdit::returnPressed, this, &CTradeAccountLoginDialog::Login);
	QPointer<CTradeAccountLoginDialog> self(this);
	m_nToken = CTradeAccountService::InstanceRef().Subscribe([self](const CTradeAccountService::CResult& result)
	{
		if (nullptr != self)
		{
			QMetaObject::invokeMethod(self, [self, result]()
			{
				if (nullptr != self)
				{
					self->HandleResult(result);
				}
			}, Qt::QueuedConnection);
		}
	});
	m_ui->accountEdit->setFocus();
}

CTradeAccountLoginDialog::~CTradeAccountLoginDialog()
{
	CTradeAccountService::InstanceRef().Unsubscribe(m_nToken);
	m_ui->passwordEdit->clear();
}

void CTradeAccountLoginDialog::SetBusy(bool bBusy)
{
	m_ui->loginButton->setEnabled(!bBusy);
	m_ui->brokerCombo->setEnabled(!bBusy);
	m_ui->siteCombo->setEnabled(!bBusy);
	m_ui->accountEdit->setEnabled(!bBusy);
	m_ui->passwordEdit->setEnabled(!bBusy);
	m_ui->loginButton->setText(bBusy ? "登录中…" : "登录");
}

void CTradeAccountLoginDialog::Login()
{
	if (0 != m_nRequestId)
	{
		return;
	}
	if ((0 > m_ui->brokerCombo->currentIndex()) || (0 > m_ui->siteCombo->currentIndex()) || m_ui->accountEdit->text().trimmed().isEmpty() || m_ui->passwordEdit->text().isEmpty())
	{
		ShowError("请选择券商、站点，并输入账户和密码");
		return;
	}
	m_nRequestId = CTradeAccountService::InstanceRef().Send(_TyRequest::TRADE_ACCOUNT_LOGIN, 0, { { "broker", m_ui->brokerCombo->currentData().toString().toStdString() }, { "site", m_ui->siteCombo->currentData().toString().toStdString() }, { "account", m_ui->accountEdit->text().trimmed().toStdString() }, { "password", m_ui->passwordEdit->text().toStdString() } });
	if (0 == m_nRequestId)
	{
		ShowError("请求未发送，请检查 XEngine 登录状态或等待当前请求完成");
		return;
	}
	ShowError(QString());
	SetBusy(true);
}

void CTradeAccountLoginDialog::HandleResult(const CTradeAccountService::CResult& result)
{
	if ((0 == m_nRequestId) || ((m_nRequestId != result.m_requestId) && (!result.m_bSessionReset)))
	{
		return;
	}
	m_nRequestId = 0;
	SetBusy(false);
	m_ui->passwordEdit->clear();
	if (!result.m_error.empty() || (result.m_bSessionReset))
	{
		ShowError(result.m_error.empty() ? "连接已断开，请重新登录 XEngine" : QString::fromStdString(result.m_error));
		return;
	}
	accept();
}

bool CTradeAccountLoginDialog::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if (((m_ui->titleFrame == pObject) || (m_ui->titleLabel == pObject)) && (QEvent::MouseButtonPress == pEvent->type()))
	{
		QMouseEvent* pMouse = static_cast<QMouseEvent*>(pEvent);
		if ((Qt::LeftButton == pMouse->button()) && (nullptr != windowHandle()))
		{
			return windowHandle()->startSystemMove();
		}
	}
	return QDialog::eventFilter(pObject, pEvent);
}

void CTradeAccountLoginDialog::ShowError(const QString& strMessage)
{
	m_ui->errorLabel->setText(strMessage);
	m_ui->errorLabel->setVisible(!strMessage.isEmpty());
	int nExtraHeight = 0;
	if (!strMessage.isEmpty())
	{
		nExtraHeight = qMax(m_ui->errorLabel->minimumHeight(), m_ui->errorLabel->heightForWidth(m_ui->errorLabel->width())) + m_ui->bodyLayout->spacing();
	}
	setFixedSize(390, 308 + nExtraHeight);
}
