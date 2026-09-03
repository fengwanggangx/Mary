#include "CMainWindow.h"

#include "../business/CWindSession.h"
#include "components/CViewProductDateInfo.h"
#include "components/CViewProducts.h"
#include "components/CViewSettings.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	struct CTreeItemInfo
	{
		CTreeItemInfo(int id, const QString& strText, const QString& strIcon, std::vector<CTreeItemInfo>&& children) : id(id), text(strText), icon(strIcon), children(std::move(children))
		{
		}

		CTreeItemInfo(int id, const QString& strText, const QString& strIcon) : id(id), text(strText), icon(strIcon)
		{
		}

		static QTreeWidgetItem* BuildItem(const CTreeItemInfo& info)
		{
			QTreeWidgetItem* pItem = new QTreeWidgetItem();
			pItem->setText(0, info.text);
			pItem->setIcon(0, QIcon(info.icon));
			pItem->setText(1, QString::number(info.id));
			for (const auto& child : info.children)
			{
				pItem->addChild(BuildItem(child));
			}
			return pItem;
		}

		int id{ -1 };
		QString text;
		QString icon;
		std::vector<CTreeItemInfo> children;
	};
}

CMainWindow::CMainWindow(QWidget* pParent) : QMainWindow(pParent), ui(new Ui::CMainWindowClass()), m_windSession(std::make_unique<CWindSession>())
{
	ui->setupUi(this);
	UIInitialized();
	ConnectSlots();
	m_windSession->Start();
}

CMainWindow::~CMainWindow()
{
	m_windSession->Stop();
	delete ui;
}

void CMainWindow::UIInitialized()
{
	static std::unordered_map<int, CTreeItemInfo> tree
	{
		{ 0, { 0, "系统设置", ":/images/setting.png" } },
		{ 1, { 1, "产品管理", ":/images/setting.png" } },
		{ 2, { 2, "产品日期", ":/images/setting.png" } },
		{ 3, { 3, "Wind 控制", ":/images/setting.png" } }
	};
	ui->treeWidget->setIndentation(10);
	for (const auto& item : tree)
	{
		ui->treeWidget->addTopLevelItem(CTreeItemInfo::BuildItem(item.second));
	}
	ui->stackedWidget->addWidget(new CViewSettings());
	ui->stackedWidget->addWidget(new CViewProducts());
	ui->stackedWidget->addWidget(new CViewProductDateInfo());
	BuildWindPage();
}

void CMainWindow::BuildWindPage()
{
	QWidget* pPage = new QWidget();
	QVBoxLayout* pLayout = new QVBoxLayout(pPage);
	QFormLayout* pStatusLayout = new QFormLayout();
	m_connectionLabel = new QLabel("未连接");
	m_versionLabel = new QLabel("未知");
	m_serviceLabel = new QLabel("未知");
	m_riskLabel = new QLabel("未知");
	m_errorLabel = new QLabel("无");
	m_errorLabel->setWordWrap(true);
	pStatusLayout->addRow("连接 / 认证", m_connectionLabel);
	pStatusLayout->addRow("Wind 版本", m_versionLabel);
	pStatusLayout->addRow("服务状态", m_serviceLabel);
	pStatusLayout->addRow("风控总开关", m_riskLabel);
	pStatusLayout->addRow("最近错误", m_errorLabel);
	pLayout->addLayout(pStatusLayout);
	m_strategyList = new QListWidget();
	pLayout->addWidget(m_strategyList);
	QHBoxLayout* pButtonLayout = new QHBoxLayout();
	m_startButton = new QPushButton("启动策略");
	m_stopButton = new QPushButton("停止策略");
	m_riskButton = new QPushButton("切换风控总开关");
	pButtonLayout->addWidget(m_startButton);
	pButtonLayout->addWidget(m_stopButton);
	pButtonLayout->addWidget(m_riskButton);
	pLayout->addLayout(pButtonLayout);
	SetControlsEnabled(false);
	ui->stackedWidget->addWidget(pPage);
}

void CMainWindow::ConnectSlots()
{
	connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &CMainWindow::OnItemSelectChanged);
	connect(m_windSession.get(), &CWindSession::ConnectionStateChanged, m_connectionLabel, &QLabel::setText);
	connect(m_windSession.get(), &CWindSession::AuthenticationChanged, this, &CMainWindow::OnAuthenticated);
	connect(m_windSession.get(), &CWindSession::ResponseReceived, this, &CMainWindow::OnResponse);
	connect(m_windSession.get(), &CWindSession::RequestFailed, this, &CMainWindow::OnRequestFailed);
	connect(m_windSession.get(), &CWindSession::RecentErrorChanged, m_errorLabel, &QLabel::setText);
	connect(m_startButton, &QPushButton::clicked, this, [this]() { SendStrategyCommand("start_strategy"); });
	connect(m_stopButton, &QPushButton::clicked, this, [this]() { SendStrategyCommand("stop_strategy"); });
	connect(m_riskButton, &QPushButton::clicked, this, [this]()
	{
		QMap<QString, QString> parameters;
		parameters.insert("enabled", m_riskEnabled ? "false" : "true");
		if (0 != m_windSession->SendCommand("set_risk_enabled", parameters))
		{
			m_riskButton->setEnabled(false);
		}
	});
}

void CMainWindow::OnItemSelectChanged()
{
	QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
	if (selected.isEmpty())
	{
		return;
	}
	QTreeWidgetItem* pItem = selected.first();
	if (nullptr == pItem)
	{
		return;
	}
	bool converted = false;
	int index = pItem->text(1).toInt(&converted);
	if (converted && (0 <= index) && (index < ui->stackedWidget->count()))
	{
		ui->stackedWidget->setCurrentIndex(index);
	}
}

void CMainWindow::OnAuthenticated(bool authenticated)
{
	SetControlsEnabled(authenticated);
	if (!authenticated)
	{
		return;
	}
	m_windSession->SendCommand("query_version");
	m_windSession->SendCommand("query_service_status");
	m_windSession->SendCommand("query_strategy_list");
	m_windSession->SendCommand("query_risk_status");
}

void CMainWindow::OnResponse(qulonglong, const QString& strCommand, const QMap<QString, QString>& result)
{
	if ("query_version" == strCommand)
	{
		m_versionLabel->setText(result.value("version", "未知"));
	}
	else if ("query_service_status" == strCommand)
	{
		m_serviceLabel->setText(result.value("status", "未知"));
	}
	else if ("query_strategy_list" == strCommand)
	{
		m_strategyList->clear();
		m_strategyList->addItems(result.value("strategies").split(',', Qt::SkipEmptyParts));
	}
	else if (("query_risk_status" == strCommand) || ("set_risk_enabled" == strCommand))
	{
		m_riskEnabled = "true" == result.value("enabled").toLower();
		m_riskLabel->setText(m_riskEnabled ? "已启用" : "已禁用");
		m_riskButton->setEnabled(true);
	}
	else if (("start_strategy" == strCommand) || ("stop_strategy" == strCommand))
	{
		SetControlsEnabled(true);
		m_windSession->SendCommand("query_strategy_list");
	}
}

void CMainWindow::OnRequestFailed(qulonglong, const QString& strCommand, const QString& strError)
{
	m_errorLabel->setText(strCommand + ": " + strError);
	SetControlsEnabled(m_windSession->IsAuthenticated());
}

void CMainWindow::SetControlsEnabled(bool enabled)
{
	m_startButton->setEnabled(enabled);
	m_stopButton->setEnabled(enabled);
	m_riskButton->setEnabled(enabled);
}

void CMainWindow::SendStrategyCommand(const QString& strCommand)
{
	QListWidgetItem* pItem = m_strategyList->currentItem();
	if (nullptr == pItem)
	{
		m_errorLabel->setText("请先选择策略");
		return;
	}
	QMap<QString, QString> parameters;
	parameters.insert("strategy", pItem->text());
	if (0 != m_windSession->SendCommand(strCommand, parameters))
	{
		m_startButton->setEnabled(false);
		m_stopButton->setEnabled(false);
	}
}
