#include "CViewAShare.h"
#include "CMarketPageController.h"
#include "ui_CViewAShare.h"

#include <QHeaderView>
#include <QMargins>
#include <QTabBar>
#include <QTimer>

namespace
{
	CurveMode CurveModeFromTab(int nIndex)
	{
		switch (nIndex)
		{
		case 0: return CurveMode::Day;
		case 1: return CurveMode::Week;
		case 2: return CurveMode::Month;
		case 3: return CurveMode::Minute5;
		case 4: return CurveMode::Minute15;
		case 5: return CurveMode::Minute30;
		case 6: return CurveMode::Minute60;
		default: return CurveMode::Day;
		}
	}
}

CViewAShare::CViewAShare(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CViewAShareClass>())
{
	m_ui->setupUi(this);
	m_ui->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_ui->table->sortByColumn(-1, Qt::AscendingOrder);
	m_ui->table->SetSearchColumns({ 0, 1, static_cast<int>(MarketQuoteColumn::PinyinFullAliases) - 1, static_cast<int>(MarketQuoteColumn::PinyinShortAliases) - 1 });
	connect(m_ui->searchEdit, &QLineEdit::textChanged, m_ui->table, &CUITable::Search);
	connect(m_ui->searchEdit, &QLineEdit::returnPressed, this, [this]()
	{
		m_ui->table->Search(m_ui->searchEdit->text());
	});
	connect(m_ui->searchButton, &QPushButton::clicked, this, [this]()
	{
		m_ui->table->Search(m_ui->searchEdit->text());
	});
	connect(m_ui->table, &CUITable::ResultsChanged, this, [this](int nCount)
	{
		m_ui->countLabel->setText(QString("共%1只").arg(nCount));
	});
	m_controller = std::make_unique<CMarketPageController>(MarketTableMode::AShare, m_ui->table, this);
	m_controller->SetCharts(m_ui->stockTitle, m_ui->priceLabel, m_ui->chartState, m_ui->intradayCurve, m_ui->candleCurve);
	m_ui->intradayLabel->hide();
	m_ui->intradayFrameLayout->insertWidget(1, m_controller->CreateIntradayControls(m_ui->intradayFrame));
	QMargins intradayMargins = m_ui->intradayFrameLayout->contentsMargins();
	int nIntradayLeft = intradayMargins.left();
	intradayMargins.setLeft(0);
	intradayMargins.setBottom(0);
	m_ui->intradayFrameLayout->setContentsMargins(intradayMargins);
	QMargins stockMargins = m_ui->stockInfoLayout->contentsMargins();
	stockMargins.setLeft(nIntradayLeft);
	m_ui->stockInfoLayout->setContentsMargins(stockMargins);
	m_ui->intradayCurve->setContentsMargins(nIntradayLeft, 0, 0, 0);
	QMargins candleMargins = m_ui->candleFrameLayout->contentsMargins();
	int nCandleLeft = candleMargins.left();
	candleMargins.setLeft(0);
	candleMargins.setTop(0);
	m_ui->candleFrameLayout->setContentsMargins(candleMargins);
	m_ui->candleCurve->setContentsMargins(nCandleLeft, 0, 0, 0);
	m_ui->chartSplitter->setHandleWidth(1);
	m_ui->periodTabs->addTab(new QWidget(m_ui->periodTabs), "5分");
	m_ui->periodTabs->addTab(new QWidget(m_ui->periodTabs), "15分");
	m_ui->periodTabs->addTab(new QWidget(m_ui->periodTabs), "30分");
	m_ui->periodTabs->addTab(new QWidget(m_ui->periodTabs), "60分");
	m_ui->periodTabs->tabBar()->setExpanding(false);
	connect(m_ui->periodTabs, &QTabWidget::currentChanged, this, [this](int nIndex)
	{
		m_controller->RequestHistory(CurveModeFromTab(nIndex));
	});
	m_ui->splitter->setStretchFactor(0, 55);
	m_ui->splitter->setStretchFactor(1, 45);
	m_ui->chartSplitter->setSizes({ 420, 420 });

	connect(m_ui->marketTabs, &QTabWidget::currentChanged, this, [this](int nIndex)
	{
		m_ui->titleLabel->setText(m_ui->marketTabs->tabText(nIndex));
		m_controller->SetMarket(nIndex);
	});
	m_controller->SetMarket(0);
}

CViewAShare::~CViewAShare() = default;

void CViewAShare::showEvent(QShowEvent* pEvent)
{
	QWidget::showEvent(pEvent);
	if (!m_bLayoutInitialized)
	{
		m_bLayoutInitialized = true;
		QTimer::singleShot(0, this, [this]()
		{
			int nWidth = m_ui->splitter->width() - m_ui->splitter->handleWidth();
			m_ui->splitter->setSizes({ nWidth * 55 / 100, nWidth * 45 / 100 });
		});
	}
}
