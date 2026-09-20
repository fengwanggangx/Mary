#include "CViewHQMarket.h"
#include "CUIStyle.h"
#include "CDataTableModel.h"
#include "../system/CSession.h"
#include "ui_CViewHQMarket.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QMetaObject>
#include <QPushButton>
#include <QTabBar>
#include <QTableWidgetItem>
#include <cmath>
#include <algorithm>
#include <functional>

CViewHQMarket::CViewHQMarket(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CViewHQMarketClass>())
{
	m_ui->setupUi(this);
	m_ui->marketTabs->tabBar()->setDrawBase(false);
	m_ui->marketTabs->tabBar()->setExpanding(false);
	m_ui->rankingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_ui->rankingTable->horizontalHeader()->setFixedHeight(24);
	ApplyTheme();
	CHQMarketService& service = CHQMarketService::InstanceRef();
	service.Initialize();
	QPointer<CViewHQMarket> safeThis(this);
	m_nQuoteTableToken = service.AddQuoteTableHandler(std::bind_front(&CViewHQMarket::HandleQuoteTable, safeThis));
	m_nSectorListToken = service.AddSectorListHandler(std::bind_front(&CViewHQMarket::HandleSectorList, safeThis));
	m_nSectorConstituentsToken = service.AddSectorConstituentsHandler(std::bind_front(&CViewHQMarket::HandleSectorConstituents, safeThis));
	connect(m_ui->marketTabs, &QTabWidget::currentChanged, this, &CViewHQMarket::OnMarketTabChanged);
	connect(m_ui->rankingTable, &QTableWidget::cellClicked, this, &CViewHQMarket::OnRankingCellClicked);
	CSession::InstanceRef().RegisterStateHandler(std::bind_front(&CViewHQMarket::HandleSessionState, safeThis));
	RefreshQuotes(service.GetQuoteTableView());
	service.SubscribeQuote(CSecurity("000001", Exchange::sse));
	service.SubscribeQuote(CSecurity("399001", Exchange::szse));
	service.SubscribeQuote(CSecurity("399006", Exchange::szse));
	RequestSectors();
}

void CViewHQMarket::HandleQuoteTable(QPointer<CViewHQMarket> safeThis, const CDataTableView& view, const CDataChangeSet&)
{
	if (safeThis.isNull())
	{
		return;
	}
	QMetaObject::invokeMethod(safeThis.data(), [safeThis, view]()
	{
		if (!safeThis.isNull())
		{
			safeThis->RefreshQuotes(view);
		}
	}, Qt::QueuedConnection);
}

void CViewHQMarket::HandleSectorList(QPointer<CViewHQMarket> safeThis, const CSectorListEvent& event)
{
	if (safeThis.isNull())
	{
		return;
	}
	QMetaObject::invokeMethod(safeThis.data(), [safeThis, event]()
	{
		if (!safeThis.isNull())
		{
			safeThis->RefreshSectors(event);
		}
	}, Qt::QueuedConnection);
}

void CViewHQMarket::HandleSectorConstituents(QPointer<CViewHQMarket> safeThis, const CSectorConstituentsEvent& event)
{
	if (safeThis.isNull())
	{
		return;
	}
	QMetaObject::invokeMethod(safeThis.data(), [safeThis, event]()
	{
		if (!safeThis.isNull())
		{
			safeThis->RefreshConstituents(event);
		}
	}, Qt::QueuedConnection);
}

void CViewHQMarket::HandleSessionState(QPointer<CViewHQMarket> safeThis, SessionState state, const std::string&)
{
	if ((SessionState::Ready != state) || safeThis.isNull())
	{
		return;
	}
	QMetaObject::invokeMethod(safeThis.data(), [safeThis]()
	{
		if (!safeThis.isNull() && (0 == safeThis->m_ui->marketTabs->currentIndex()))
		{
			safeThis->m_bOverviewRequested = false;
			safeThis->RequestSectors();
		}
	}, Qt::QueuedConnection);
}

CViewHQMarket::~CViewHQMarket()
{
	CHQMarketService::InstanceRef().RemoveQuoteTableHandler(m_nQuoteTableToken);
	CHQMarketService::InstanceRef().RemoveSectorListHandler(m_nSectorListToken);
	CHQMarketService::InstanceRef().RemoveSectorConstituentsHandler(m_nSectorConstituentsToken);
}

void CViewHQMarket::OnMarketTabChanged(int nIndex)
{
	if (0 == nIndex)
	{
		RequestSectors();
	}
	else
	{
		m_bOverviewRequested = false;
	}
}

void CViewHQMarket::OnRankingCellClicked(int nRow, int)
{
	QTableWidgetItem* pItem = m_ui->rankingTable->item(nRow, 1);
	if (nullptr != pItem)
	{
		SelectSector(pItem->data(Qt::UserRole).toString());
	}
}

void CViewHQMarket::HandleSectorButtonClicked()
{
	QPushButton* pButton = qobject_cast<QPushButton*>(sender());
	if (nullptr != pButton)
	{
		SelectSector(pButton->property("sectorCode").toString());
	}
}

void CViewHQMarket::RequestSectors()
{
	if (m_bOverviewRequested)
	{
		return;
	}
	if (!CSession::InstanceRef().IsAuthenticated())
	{
		m_ui->sectorState->setText("等待行情服务连接");
		return;
	}
	m_bOverviewRequested = true;
	m_ui->sectorState->setText("正在加载行业板块…");
	CHQMarketService::InstanceRef().QuerySectors(SectorType::industry);
}

void CViewHQMarket::RefreshSectors(const CSectorListEvent& event)
{
	while (nullptr != m_ui->sectorGrid->itemAt(0))
	{
		QLayoutItem* pItem = m_ui->sectorGrid->takeAt(0);
		delete pItem->widget();
		delete pItem;
	}
	m_ui->rankingTable->setRowCount(0);
	if (!event.m_strError.empty())
	{
		m_ui->sectorState->setText(QString::fromStdString(event.m_strError));
		return;
	}
	m_sectors = event.m_sectors;
	std::sort(m_sectors.begin(), m_sectors.end(), [](const CSectorInfo& left, const CSectorInfo& right)
			  { return left.m_fChangePercent > right.m_fChangePercent; });
	if (m_sectors.empty())
	{
		m_ui->sectorState->setText("暂无行业板块数据");
		return;
	}
	m_ui->sectorState->setText(QString("共%1个行业").arg(m_sectors.size()));
	int nTileCount = (std::min)(9, static_cast<int>(m_sectors.size()));
	for (int nIndex = 0; nTileCount > nIndex; ++nIndex)
	{
		const CSectorInfo& sector = m_sectors[static_cast<std::size_t>(nIndex)];
		QPushButton* pButton = new QPushButton(QString("%1\n%2%3%").arg(QString::fromStdString(sector.m_strName), 0.0 <= sector.m_fChangePercent ? "+" : "", QString::number(sector.m_fChangePercent, 'f', 2)), this);
		pButton->setCheckable(true);
		pButton->setProperty("sectorTile", true);
		pButton->setProperty("negative", 0.0 > sector.m_fChangePercent);
		pButton->setProperty("sectorCode", QString::fromStdString(sector.m_strCode));
		connect(pButton, &QPushButton::clicked, this, &CViewHQMarket::HandleSectorButtonClicked);
		m_ui->sectorGrid->addWidget(pButton, nIndex / 5, nIndex % 5);
	}
	int nRankingCount = (std::min)(5, static_cast<int>(m_sectors.size()));
	m_ui->rankingTable->setRowCount(nRankingCount);
	for (int nIndex = 0; nRankingCount > nIndex; ++nIndex)
	{
		const CSectorInfo& sector = m_sectors[static_cast<std::size_t>(nIndex)];
		QTableWidgetItem* pRank = new QTableWidgetItem(QString::number(nIndex + 1));
		QTableWidgetItem* pName = new QTableWidgetItem(QString::fromStdString(sector.m_strName));
		QTableWidgetItem* pPercent = new QTableWidgetItem(QString("%1%2%").arg(0.0 <= sector.m_fChangePercent ? "+" : "", QString::number(sector.m_fChangePercent, 'f', 2)));
		pName->setData(Qt::UserRole, QString::fromStdString(sector.m_strCode));
		m_ui->rankingTable->setItem(nIndex, 0, pRank);
		m_ui->rankingTable->setItem(nIndex, 1, pName);
		m_ui->rankingTable->setItem(nIndex, 2, pPercent);
	}
}

void CViewHQMarket::SelectSector(const QString& strSectorCode)
{
	if (strSectorCode.isEmpty())
	{
		return;
	}
	m_strSelectedSectorCode = strSectorCode;
	for (int nIndex = 0; m_ui->sectorGrid->count() > nIndex; ++nIndex)
	{
		QPushButton* pButton = qobject_cast<QPushButton*>(m_ui->sectorGrid->itemAt(nIndex)->widget());
		if (nullptr != pButton)
		{
			pButton->setChecked(strSectorCode == pButton->property("sectorCode").toString());
		}
	}
	m_ui->sectorState->setText("正在加载成分股…");
	CHQMarketService::InstanceRef().QuerySectorConstituents(SectorType::industry, strSectorCode.toStdString());
}

void CViewHQMarket::RefreshConstituents(const CSectorConstituentsEvent& event)
{
	if (!event.m_strError.empty())
	{
		m_ui->sectorState->setText(QString::fromStdString(event.m_strError));
		return;
	}
	if (m_strSelectedSectorCode != QString::fromStdString(event.m_sector.m_strCode))
	{
		return;
	}
	m_ui->sectorState->setText(QString("%1 · %2只成分股").arg(QString::fromStdString(event.m_sector.m_strName)).arg(event.m_securities.size()));
	m_ui->constituentsPage->SetSector(event.m_sector, event.m_securities);
}

void CViewHQMarket::RefreshQuotes(const CDataTableView& view)
{
	CDataTableModel model;
	model.SetView(view, CDataChangeSet{});
	QList<QLabel*> prices{ m_ui->shanghaiPrice, m_ui->shenzhenPrice, m_ui->chinextPrice };
	QList<QLabel*> changes{ m_ui->shanghaiChange, m_ui->shenzhenChange, m_ui->chinextChange };
	QStringList indices{ "000001.SSE", "399001.SZSE", "399006.SZSE" };
	for (const auto& label : prices)
	{
		label->setText("--");
	}
	for (const auto& label : changes)
	{
		label->setText("--");
	}
	int nRising = 0;
	int nFalling = 0;
	int nFlat = 0;
	QVector<int> distribution(13, 0);
	for (int nRow = 0; model.rowCount() > nRow; ++nRow)
	{
		QString strSecurity = model.index(nRow, 0).data().toString();
		double fPrice = model.index(nRow, 2).data().toDouble();
		double fPreClose = model.index(nRow, 5).data().toDouble();
		if ((0.0 >= fPrice) || (0.0 >= fPreClose) || ("交易中" != model.index(nRow, 7).data().toString()))
		{
			continue;
		}
		double fPercent = model.index(nRow, 4).data().toDouble();
		int nIndex = static_cast<int>(indices.indexOf(strSecurity));
		if (0 <= nIndex)
		{
			prices[nIndex]->setText(QString::number(fPrice, 'f', 2));
			changes[nIndex]->setText(QString("%1%2%  %1%3").arg(0.0 <= fPercent ? "+" : "").arg(fPercent, 0, 'f', 2).arg(fPrice - fPreClose, 0, 'f', 2));
			prices[nIndex]->setProperty("rising", 0.0 <= fPercent);
			changes[nIndex]->setProperty("rising", 0.0 <= fPercent);
			UIStyle::Refresh(*prices[nIndex]);
			UIStyle::Refresh(*changes[nIndex]);
			continue;
		}
		QString strMarket = model.index(nRow, 10).data().toString();
		if (("沪A" != strMarket) && ("深A" != strMarket) && ("创业板" != strMarket) && ("科创板" != strMarket))
		{
			continue;
		}
		if (0.0 < fPercent)
		{
			++nRising;
		}
		else if (0.0 > fPercent)
		{
			++nFalling;
		}
		else
		{
			++nFlat;
		}
		int nBucket = std::clamp(static_cast<int>(std::floor(fPercent)) + 6, 0, 12);
		++distribution[nBucket];
	}
	bool bHasQuotes = 0 < (nRising + nFalling + nFlat);
	m_ui->metric1Value->setText(bHasQuotes ? QString::number(nRising) : "--");
	m_ui->metric2Value->setText(bHasQuotes ? QString::number(nFalling) : "--");
	m_ui->metric3Value->setText(bHasQuotes ? QString::number(nFlat) : "--");
	m_ui->distributionChart->SetValues(bHasQuotes ? distribution : QVector<int>{});
}

void CViewHQMarket::changeEvent(QEvent* pEvent)
{
	QWidget::changeEvent(pEvent);
	if (QEvent::PaletteChange == pEvent->type())
	{
		ApplyTheme();
	}
}

void CViewHQMarket::ApplyTheme()
{
	bool bDarkTheme = 128 > qApp->palette().color(QPalette::Window).lightness();
	if (property("darkTheme").isValid() && (bDarkTheme == property("darkTheme").toBool()))
	{
		return;
	}
	setProperty("darkTheme", bDarkTheme);
	UIStyle::Apply(*this, ":/styles/market-overview.qss");
	UIStyle::Refresh(*this);
}
