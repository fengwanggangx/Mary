#include "CViewStrategySettings.h"
#include "CUIStyle.h"
#include "ui_CViewStrategySettings.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QTableWidget>
#include <QTabBar>

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
	m_ui->strategyCountValue->setText("6");
	m_ui->runningCountValue->setText("3");
	m_ui->totalProfitValue->setText("+28,635.80 元");
	m_ui->todayProfitValue->setText("+1,168.46 元");
	AppendRow(m_pInstances, { "茅台趋势01", "双均线", "模拟账户", "600519.SH", "模拟", "运行中", "100股", "+682.35", "09:28:15" });
	AppendRow(m_pInstances, { "平安网格01", "网格交易", "模拟账户", "000001.SZ", "模拟", "运行中", "2,000股", "+432.18", "09:31:42" });
	AppendRow(m_pInstances, { "ETF突破01", "突破策略", "实盘账户", "510300.SH", "实盘", "已就绪", "0", "0.00", "09:20:36" });
	AppendRow(m_pInstances, { "五粮液趋势", "双均线", "模拟账户", "000858.SZ", "模拟", "已停止", "0", "-218.65", "08:56:21" });
	AppendRow(m_pInstances, { "招行回归01", "均值回归", "模拟账户", "600036.SH", "模拟", "运行中", "1,000股", "+216.30", "09:15:33" });
	AppendRow(m_pInstances, { "数据测试01", "网格交易", "模拟账户", "000001.SZ", "模拟", "异常", "0", "+56.28", "09:12:08" });
	AppendRow(m_ui->parametersTable, { "单笔数量", "—", "单次下单数量" });
	AppendRow(m_ui->parametersTable, { "止损比例", "—", "最大亏损止损比例" });
	AppendRow(m_ui->logsTable, { "—", "信息", "策略页面已加载，尚未连接策略执行引擎" });
	m_ui->detailSplitter->setStretchFactor(0, 1);
	m_ui->detailSplitter->setStretchFactor(1, 1);
	connect(m_pSearch, &QLineEdit::textChanged, this, &CViewStrategySettings::RefreshFilter);
	connect(m_pStatus, &QComboBox::currentIndexChanged, this, &CViewStrategySettings::RefreshFilter);
	connect(m_pInstances, &QTableWidget::itemSelectionChanged, this, &CViewStrategySettings::RefreshDetails);
	m_pInstances->selectRow(0);
	ApplyTheme();
}

CViewStrategySettings::~CViewStrategySettings() = default;

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
		return;
	}
	m_pDetails->setText(QString("%1 · %2 · %3 · %4\n持仓：%5   今日盈亏：%6 元   执行引擎：未接入（UI 展示）").arg(m_pInstances->item(nRow, 0)->text(), m_pInstances->item(nRow, 1)->text(), m_pInstances->item(nRow, 3)->text(), m_pInstances->item(nRow, 5)->text(), m_pInstances->item(nRow, 6)->text(), m_pInstances->item(nRow, 7)->text()));
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
