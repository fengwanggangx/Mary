#include "CViewStrategySettings.h"
#include "CUIStyle.h"
#include "CStrategyEditDialog.h"
#include "../system/CSession.h"
#include "ui_CViewStrategySettings.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QTableWidget>
#include <QTabBar>
#include <QPointer>
#include <QMetaObject>
#include <QTimer>
#include <QMessageBox>

namespace
{
	void AppendRow(QTableWidget* pTable, const QStringList& values)
	{
		int nRow = pTable->rowCount();
		pTable->insertRow(nRow);
		for (int nColumn = 0; values.size() > nColumn; ++nColumn)
		{
			pTable->setItem(nRow, nColumn, new QTableWidgetItem(values[nColumn]));
		}
	}
} // namespace

CViewStrategySettings::CViewStrategySettings(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CViewStrategySettingsClass>())
{
	m_ui->setupUi(this);
	m_pInstances = m_ui->instancesTable;
	m_pDetails = m_ui->detailsLabel;
	m_pSearch = m_ui->searchEdit;
	m_pStatus = m_ui->statusCombo;
	for (const auto& tabs : QList<QTabWidget*>{ m_ui->strategyTabs, m_ui->detailsTabs })
	{
		tabs->tabBar()->setMinimumHeight(38);
		tabs->tabBar()->setDrawBase(false);
		tabs->tabBar()->setExpanding(false);
	}
	for (const auto& table : QList<QTableWidget*>{ m_pInstances, m_ui->parametersTable, m_ui->logsTable })
	{
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	}
	m_ui->detailSplitter->setStretchFactor(0, 1);
	m_ui->detailSplitter->setStretchFactor(1, 1);
	connect(m_pSearch, &QLineEdit::textChanged, this, &CViewStrategySettings::RefreshFilter);
	connect(m_pStatus, &QComboBox::currentIndexChanged, this, &CViewStrategySettings::RefreshFilter);
	connect(m_pInstances, &QTableWidget::itemSelectionChanged, this, &CViewStrategySettings::RefreshDetails);
	ApplyTheme();
	CStrategyService& service = CStrategyService::InstanceRef();
	service.Initialize();
	QPointer<CViewStrategySettings> safeThis(this);
	m_nQueryToken = service.AddQueryHandler([safeThis](const CStrategyService::_TyStrategyList& strategies, const std::string& strError)
	{
		if (!safeThis.isNull())
		{
			QMetaObject::invokeMethod(safeThis.data(), [safeThis, strategies, strError]()
			{
				if (!safeThis.isNull())
				{
					safeThis->RefreshStrategies(strategies, strError);
				}
			}, Qt::QueuedConnection);
		}
	});
	connect(m_ui->newButton, &QPushButton::clicked, this, [this]()
	{
		EditStrategy(true);
	});
	connect(m_ui->modifyButton, &QPushButton::clicked, this, [this]()
	{
		EditStrategy(false);
	});
	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &CViewStrategySettings::RefreshConnection);
	timer->start(1000);
	RefreshConnection();
	if (service.IsCacheValid())
	{
		RefreshStrategies(service.GetStrategies(), std::string());
	}
	if (CSession::InstanceRef().IsAuthenticated() && !service.QueryStrategies())
	{
		m_ui->strategyCountHint->setText("策略查询未发送");
	}
}

CViewStrategySettings::~CViewStrategySettings()
{
	CStrategyService::InstanceRef().RemoveQueryHandler(m_nQueryToken);
}

void CViewStrategySettings::RefreshFilter()
{
	QString strKeyword = m_pSearch->text().trimmed();
	for (int nRow = 0; nRow < m_pInstances->rowCount(); ++nRow)
	{
		bool bMatches = strKeyword.isEmpty();
		for (int nColumn = 0; nColumn < 4; ++nColumn)
		{
			bMatches = bMatches || m_pInstances->item(nRow, nColumn)->text().contains(strKeyword, Qt::CaseInsensitive);
		}
		bMatches = bMatches && ((0 == m_pStatus->currentIndex()) || (m_pStatus->currentText() == m_pInstances->item(nRow, 5)->text()));
		m_pInstances->setRowHidden(nRow, !bMatches);
	}
	RefreshDetails();
}

void CViewStrategySettings::RefreshDetails()
{
	int nRow = m_pInstances->currentRow();
	if ((0 > nRow) || m_pInstances->isRowHidden(nRow))
	{
		m_pDetails->setText(QStringLiteral("请选择策略实例"));
		m_ui->parametersTable->setRowCount(0);
		RefreshConnection();
		return;
	}
	m_pDetails->setText(QString("%1 · %2 · %3\n配置已加载；运行状态、持仓和收益暂无协议数据").arg(m_pInstances->item(nRow, 0)->text(), m_pInstances->item(nRow, 1)->text(), m_pInstances->item(nRow, 3)->text()));
	m_ui->parametersTable->setRowCount(0);
	for (const auto& [strName, strValue] : m_strategies[nRow].parameters())
	{
		AppendRow(m_ui->parametersTable, { QString::fromStdString(strName), QString::fromStdString(strValue), QString() });
	}
	RefreshConnection();
}

void CViewStrategySettings::changeEvent(QEvent* pEvent)
{
	QWidget::changeEvent(pEvent);
	if (QEvent::PaletteChange == pEvent->type())
	{
		ApplyTheme();
	}
}

void CViewStrategySettings::ApplyTheme()
{
	bool bDarkTheme = 128 > qApp->palette().color(QPalette::Window).lightness();
	if (property("darkTheme").isValid() && (bDarkTheme == property("darkTheme").toBool()))
	{
		return;
	}
	setProperty("darkTheme", bDarkTheme);
	UIStyle::Apply(*this, ":/styles/strategy-pages.qss");
	UIStyle::Refresh(*this);
	m_ui->runningCountValue->ensurePolished();
	m_ui->totalProfitValue->ensurePolished();
	QColor risingColor = m_ui->totalProfitValue->palette().color(QPalette::WindowText);
	QColor fallingColor = m_ui->runningCountValue->palette().color(QPalette::WindowText);
	for (int nRow = 0; m_pInstances->rowCount() > nRow; ++nRow)
	{
		QTableWidgetItem* profit = m_pInstances->item(nRow, 7);
		profit->setForeground(profit->text().startsWith('-') ? fallingColor : risingColor);
	}
}

void CViewStrategySettings::RefreshStrategies(const CStrategyService::_TyStrategyList& strategies, const std::string& strError)
{
	if (!strError.empty())
	{
		m_ui->strategyCountHint->setText(QString::fromStdString(strError));
		m_pInstances->setRowCount(0);
		m_strategies.clear();
		m_ui->strategyCountValue->setText("--");
		RefreshDetails();
		return;
	}
	m_strategies = strategies;
	m_pInstances->setRowCount(0);
	for (const auto& strategy : m_strategies)
	{
		QStringList securities;
		for (const auto& subscription : strategy.subscriptions())
		{
			securities.append(QString::fromStdString(subscription.security() + "." + subscription.exchange()));
		}
		AppendRow(m_pInstances, { QString::fromStdString(strategy.strategy_name()), QString::fromStdString(strategy.strategy_type()), "--", securities.join(", "), "--", strategy.enabled() ? "已启用" : "已禁用", "--", "--", "--" });
	}
	m_ui->strategyCountValue->setText(QString::number(m_strategies.size()));
	m_ui->strategyCountHint->setText("服务器配置");
	RefreshFilter();
	if (0 < m_pInstances->rowCount())
	{
		m_pInstances->selectRow(0);
	}
	RefreshDetails();
}

void CViewStrategySettings::RefreshConnection()
{
	bool bConnected = CSession::InstanceRef().IsAuthenticated();
	m_ui->newButton->setEnabled(bConnected);
	int nRow = m_pInstances->currentRow();
	m_ui->modifyButton->setEnabled(bConnected && (0 <= nRow) && (m_strategies.size() > static_cast<std::size_t>(nRow)) && !m_pInstances->isRowHidden(nRow));
	m_ui->runtimeLabel->setText(bConnected ? "连接正常；运行状态、信号、持仓和收益暂无协议数据" : "连接不可用，等待登录或重连");
}

void CViewStrategySettings::EditStrategy(bool bNew)
{
	if (!CSession::InstanceRef().IsAuthenticated())
	{
		return;
	}
	CStrategyEditDialog dialog(this);
	if (!bNew)
	{
		int nRow = m_pInstances->currentRow();
		if ((0 > nRow) || (m_strategies.size() <= static_cast<std::size_t>(nRow)))
		{
			return;
		}
		dialog.SetStrategy(m_strategies[nRow]);
	}
	if (QDialog::Accepted != dialog.exec())
	{
		return;
	}
	request::StrategyInfo strategy = dialog.GetStrategy();
	bool bSent = bNew ? CStrategyService::InstanceRef().AddStrategy(strategy) : CStrategyService::InstanceRef().ModifyStrategy(strategy);
	if (!bSent)
	{
		QMessageBox::warning(this, "请求未发送", "请检查连接及策略配置");
	}
	else
	{
		m_ui->strategyCountHint->setText("配置请求已发送，等待服务器确认");
	}
}
