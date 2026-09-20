#include "CViewConstituents.h"
#include "CMarketPageController.h"
#include "ui_CViewConstituents.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QHeaderView>

CViewConstituents::CViewConstituents(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CViewConstituentsClass>())
{
	m_ui->setupUi(this);
	m_ui->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_ui->table->sortByColumn(-1, Qt::AscendingOrder);
	m_ui->table->SetSearchColumns({ 0, 1 });
	connect(m_ui->searchEdit, &QLineEdit::textChanged, m_ui->table, &CUITable::Search);
	connect(m_ui->searchEdit, &QLineEdit::returnPressed, this, [this]()
			{ m_ui->table->Search(m_ui->searchEdit->text()); });
	connect(m_ui->searchButton, &QPushButton::clicked, this, [this]()
			{ m_ui->table->Search(m_ui->searchEdit->text()); });
	connect(m_ui->table, &CUITable::ResultsChanged, this, [this](int nCount)
			{ m_ui->countLabel->setText((m_strSector.isEmpty() ? QString() : m_strSector + " · ") + QString("共%1只").arg(nCount)); });
	m_controller = std::make_unique<CMarketPageController>(MarketTableMode::Constituents, m_ui->table, this);

	ApplyTheme();
}

CViewConstituents::~CViewConstituents() = default;

void CViewConstituents::SetSector(const CSectorInfo& sector, const std::vector<CSecurity>& securities)
{
	m_strSector = QString::fromStdString(sector.m_strName);
	m_controller->SetConstituents(securities);
}

void CViewConstituents::changeEvent(QEvent* pEvent)
{
	QWidget::changeEvent(pEvent);
	if (QEvent::PaletteChange == pEvent->type())
	{
		ApplyTheme();
	}
}

void CViewConstituents::ApplyTheme()
{
	setProperty("darkTheme", 128 > qApp->palette().color(QPalette::Window).lightness());
	QFile file(":/styles/market-pages.qss");
	if (file.open(QIODevice::ReadOnly))
	{
		setStyleSheet(QString::fromUtf8(file.readAll()));
	}
}
