#include "CViewStrategySettings.h"

#include <QComboBox>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace
{
	QTableWidget* CreateTable(const QStringList& headers, QWidget* pParent)
	{
		QTableWidget* pTable = new QTableWidget(0, headers.size(), pParent);
		pTable->setHorizontalHeaderLabels(headers);
		pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		pTable->setSelectionMode(QAbstractItemView::SingleSelection);
		pTable->setAlternatingRowColors(true);
		pTable->verticalHeader()->hide();
		pTable->verticalHeader()->setDefaultSectionSize(30);
		pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		pTable->horizontalHeader()->setStretchLastSection(true);
		return pTable;
	}

	void AppendRow(QTableWidget* pTable, const QStringList& values)
	{
		int row = pTable->rowCount();
		pTable->insertRow(row);
		for (int column = 0; column < values.size(); ++column)
		{
			pTable->setItem(row, column, new QTableWidgetItem(values[column]));
		}
	}
}

CViewStrategySettings::CViewStrategySettings(QWidget* pParent) : QWidget(pParent)
{
	setObjectName("strategyManagementPage");
	QVBoxLayout* pRoot = new QVBoxLayout(this);
	pRoot->setContentsMargins(8, 8, 8, 8);
	QTabWidget* pTabs = new QTabWidget(this);
	pTabs->addTab(CreateInstancePage(), QStringLiteral("策略实例"));
	for (const auto& title : QStringList{ "回测分析", "策略模板", "运行日志" })
	{
		pTabs->addTab(new QWidget(pTabs), title);
	}
	pRoot->addWidget(pTabs);
}

QWidget* CViewStrategySettings::CreateInstancePage()
{
	QWidget* pPage = new QWidget(this);
	QVBoxLayout* pLayout = new QVBoxLayout(pPage);
	pLayout->setContentsMargins(8, 8, 8, 8);
	QHBoxLayout* pMetrics = new QHBoxLayout();
	QStringList titles{ "策略数量", "运行中", "总收益", "今日收益" };
	QStringList values{ "6", "3", "+28,635.80 元", "+1,168.46 元" };
	for (int index = 0; index < titles.size(); ++index)
	{
		QFrame* pCard = new QFrame(pPage);
		pCard->setFrameShape(QFrame::StyledPanel);
		QVBoxLayout* pCardLayout = new QVBoxLayout(pCard);
		pCardLayout->addWidget(new QLabel(titles[index], pCard));
		QLabel* pValue = new QLabel(values[index], pCard);
		QFont font = pValue->font();
		font.setPointSize(20);
		font.setBold(true);
		pValue->setFont(font);
		if (1 == index)
		{
			pValue->setStyleSheet("color: #00bfa5;");
		}
		else if (2 <= index)
		{
			pValue->setStyleSheet("color: #ef4354;");
		}
		pCardLayout->addWidget(pValue);
		pCardLayout->addWidget(new QLabel(QStringLiteral("UI 展示数据 · 未接入服务"), pCard));
		pMetrics->addWidget(pCard, 1);
	}
	pLayout->addLayout(pMetrics);
	QHBoxLayout* pToolbar = new QHBoxLayout();
	for (const auto& title : QStringList{ "+ 新建实例", "初始化", "启动", "停止", "更多" })
	{
		QPushButton* pButton = new QPushButton(title, pPage);
		pButton->setEnabled(false);
		pButton->setToolTip(QStringLiteral("仅 UI 预览，暂未接入策略服务"));
		pToolbar->addWidget(pButton);
	}
	pToolbar->addStretch();
	m_pSearch = new QLineEdit(pPage);
	m_pSearch->setPlaceholderText(QStringLiteral("搜索实例、标的或模板"));
	m_pSearch->setClearButtonEnabled(true);
	pToolbar->addWidget(m_pSearch, 1);
	m_pStatus = new QComboBox(pPage);
	m_pStatus->addItems({ "全部状态", "运行中", "已就绪", "已停止", "异常" });
	pToolbar->addWidget(m_pStatus);
	pLayout->addLayout(pToolbar);
	QSplitter* pSplitter = new QSplitter(Qt::Vertical, pPage);
	pSplitter->setChildrenCollapsible(false);
	m_pInstances = CreateTable({ "实例名称", "策略模板", "账户", "交易标的", "模式", "状态", "持仓", "今日盈亏", "更新时间" }, pSplitter);
	AppendRow(m_pInstances, { "茅台趋势01", "双均线", "模拟账户", "600519.SH", "模拟", "运行中", "100股", "+682.35", "09:28:15" });
	AppendRow(m_pInstances, { "平安网格01", "网格交易", "模拟账户", "000001.SZ", "模拟", "运行中", "2,000股", "+432.18", "09:31:42" });
	AppendRow(m_pInstances, { "ETF突破01", "突破策略", "实盘账户", "510300.SH", "实盘", "已就绪", "0", "0.00", "09:20:36" });
	AppendRow(m_pInstances, { "五粮液趋势", "双均线", "模拟账户", "000858.SZ", "模拟", "已停止", "0", "-218.65", "08:56:21" });
	AppendRow(m_pInstances, { "招行回归01", "均值回归", "模拟账户", "600036.SH", "模拟", "运行中", "1,000股", "+216.30", "09:15:33" });
	AppendRow(m_pInstances, { "数据测试01", "网格交易", "模拟账户", "000001.SZ", "模拟", "异常", "0", "+56.28", "09:12:08" });
	for (int row = 0; row < m_pInstances->rowCount(); ++row)
	{
		m_pInstances->item(row, 7)->setForeground(QColor(m_pInstances->item(row, 7)->text().startsWith('-') ? "#00bfa5" : "#ef4354"));
	}
	pSplitter->addWidget(m_pInstances);
	QWidget* pDetailPage = new QWidget(pSplitter);
	QVBoxLayout* pDetailLayout = new QVBoxLayout(pDetailPage);
	m_pDetails = new QLabel(pDetailPage);
	m_pDetails->setWordWrap(true);
	pDetailLayout->addWidget(m_pDetails);
	QTabWidget* pDetailsTabs = new QTabWidget(pDetailPage);
	QWidget* pParameters = new QWidget(pDetailsTabs);
	QVBoxLayout* pParametersLayout = new QVBoxLayout(pParameters);
	QLabel* pHint = new QLabel(QStringLiteral("参数预览 · 仅展示结构，实际参数待接入策略服务"), pParameters);
	pParametersLayout->addWidget(pHint);
	QTableWidget* pParametersTable = CreateTable({ "参数", "当前值", "说明" }, pParameters);
	AppendRow(pParametersTable, { "单笔数量", "—", "单次下单数量" });
	AppendRow(pParametersTable, { "止损比例", "—", "最大亏损止损比例" });
	pParametersLayout->addWidget(pParametersTable, 1);
	QPushButton* pModify = new QPushButton(QStringLiteral("修改参数（暂未接入）"), pParameters);
	pModify->setEnabled(false);
	pParametersLayout->addWidget(pModify);
	QLabel* pRuntime = new QLabel(QStringLiteral("运行概况：最新信号 —    最新价格 —    上次心跳 —    数据连接：未接入"), pParameters);
	pRuntime->setWordWrap(true);
	pParametersLayout->addWidget(pRuntime);
	pParametersLayout->addWidget(new QLabel(QStringLiteral("最近日志（UI 展示）"), pParameters));
	QTableWidget* pLogs = CreateTable({ "时间", "级别", "内容" }, pParameters);
	AppendRow(pLogs, { "—", "信息", "策略页面已加载，尚未连接策略执行引擎" });
	pLogs->setMaximumHeight(90);
	pParametersLayout->addWidget(pLogs);
	pDetailsTabs->addTab(pParameters, QStringLiteral("参数"));
	for (const auto& title : QStringList{ "运行变量", "委托", "成交", "日志" })
	{
		pDetailsTabs->addTab(new QWidget(pDetailsTabs), title);
	}
	pDetailLayout->addWidget(pDetailsTabs, 1);
	pSplitter->addWidget(pDetailPage);
	pSplitter->setStretchFactor(0, 1);
	pSplitter->setStretchFactor(1, 1);
	pLayout->addWidget(pSplitter, 1);
	connect(m_pSearch, &QLineEdit::textChanged, this, &CViewStrategySettings::RefreshFilter);
	connect(m_pStatus, &QComboBox::currentIndexChanged, this, &CViewStrategySettings::RefreshFilter);
	connect(m_pInstances, &QTableWidget::itemSelectionChanged, this, &CViewStrategySettings::RefreshDetails);
	m_pInstances->selectRow(0);
	return pPage;
}

void CViewStrategySettings::RefreshFilter()
{
	QString keyword = m_pSearch->text().trimmed();
	for (int row = 0; row < m_pInstances->rowCount(); ++row)
	{
		bool matches = keyword.isEmpty();
		for (int column = 0; column < 4; ++column)
		{
			matches = matches || m_pInstances->item(row, column)->text().contains(keyword, Qt::CaseInsensitive);
		}
		matches = matches && ((0 == m_pStatus->currentIndex()) || (m_pStatus->currentText() == m_pInstances->item(row, 5)->text()));
		m_pInstances->setRowHidden(row, !matches);
	}
	RefreshDetails();
}

void CViewStrategySettings::RefreshDetails()
{
	int row = m_pInstances->currentRow();
	if ((0 > row) || m_pInstances->isRowHidden(row))
	{
		m_pDetails->setText(QStringLiteral("请选择策略实例"));
		return;
	}
	m_pDetails->setText(QString("%1 · %2 · %3 · %4\n持仓：%5   今日盈亏：%6 元   执行引擎：未接入（UI 展示）").arg(m_pInstances->item(row, 0)->text(), m_pInstances->item(row, 1)->text(), m_pInstances->item(row, 3)->text(), m_pInstances->item(row, 5)->text(), m_pInstances->item(row, 6)->text(), m_pInstances->item(row, 7)->text()));
}
