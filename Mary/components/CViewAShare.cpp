#include "CViewAShare.h"
#include "CMarketPageController.h"
#include "ui_CViewAShare.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QHeaderView>
#include <QTimer>

CViewAShare::CViewAShare(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CViewAShareClass>())
{
	m_ui->setupUi(this);
	m_ui->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_ui->table->sortByColumn(-1, Qt::AscendingOrder);
	m_ui->table->SetSearchColumns({ 0, 1 });
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
	connect(m_ui->periodTabs, &QTabWidget::currentChanged, this, [this](int nIndex)
	{
		m_controller->RequestHistory(0 == nIndex ? CurveMode::Day : 1 == nIndex ? CurveMode::Week
																				: CurveMode::Month);
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
	ApplyTheme();
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

void CViewAShare::changeEvent(QEvent* pEvent)
{
	QWidget::changeEvent(pEvent);
	if (QEvent::PaletteChange == pEvent->type())
	{
		ApplyTheme();
	}
}

void CViewAShare::ApplyTheme()
{
	setProperty("darkTheme", 128 > qApp->palette().color(QPalette::Window).lightness());
	QFile file(":/styles/market-pages.qss");
	if (file.open(QIODevice::ReadOnly))
	{
		setStyleSheet(QString::fromUtf8(file.readAll()));
	}
}
