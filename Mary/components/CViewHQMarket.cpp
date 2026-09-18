#include "CViewHQMarket.h"
#include "CUIStyle.h"
#include "CDataTableModel.h"
#include "ui_CViewHQMarket.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QMetaObject>
#include <QPointer>
#include <QTabBar>
#include <cmath>
#include <algorithm>

CViewHQMarket::CViewHQMarket(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CViewHQMarketClass>())
{
	m_ui->setupUi(this);
	m_ui->marketTabs->tabBar()->setDrawBase(false);
	m_ui->marketTabs->tabBar()->setExpanding(false);
	m_ui->rankingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_ui->rankingTable->horizontalHeader()->setFixedHeight(24);
	m_ui->constituentsPage->SetSector("行业数据未提供");
	ApplyTheme();
	CHQMarketService& service = CHQMarketService::InstanceRef();
	service.Initialize();
	QPointer<CViewHQMarket> safeThis(this);
	m_nQuoteTableToken = service.AddQuoteTableHandler([safeThis](const CDataTableView& view, const CDataChangeSet&)
	{
		if (!safeThis.isNull())
		{
			QMetaObject::invokeMethod(safeThis.data(), [safeThis, view]()
			{
				if (!safeThis.isNull())
				{
					safeThis->RefreshQuotes(view);
				}
			}, Qt::QueuedConnection);
		}
	});
	RefreshQuotes(service.GetQuoteTableView());
	service.SubscribeQuote(CSecurity("000001", Exchange::sse));
	service.SubscribeQuote(CSecurity("399001", Exchange::szse));
	service.SubscribeQuote(CSecurity("399006", Exchange::szse));
}

CViewHQMarket::~CViewHQMarket()
{
	CHQMarketService::InstanceRef().RemoveQuoteTableHandler(m_nQuoteTableToken);
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
