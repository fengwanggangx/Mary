#include "CViewWatchlist.h"

#include "../system/CMarketQuoteService.h"
#include "ui_CViewWatchlist.h"

#include <QHeaderView>
#include <QMetaObject>
#include <QPointer>
#include <QTableWidgetItem>

#include <array>
#include <cmath>

namespace
{
	struct CWatchlistItem
	{
		const char* m_pSecurity;
		const char* m_pName;
	};

	const std::array<CWatchlistItem, 5> s_watchlist
	{
		CWatchlistItem{ "600519.SSE", "贵州茅台" },
		CWatchlistItem{ "000001.SZSE", "平安银行" },
		CWatchlistItem{ "300750.SZSE", "宁德时代" },
		CWatchlistItem{ "601318.SSE", "中国平安" },
		CWatchlistItem{ "000858.SZSE", "五粮液" }
	};
}

CViewWatchlist::CViewWatchlist(QWidget* pParent) : QWidget(pParent), ui(new Ui::CViewWatchlistClass())
{
	ui->setupUi(this);
	InitializeUI();
	InitializeWatchlist();
	BindService();
}

CViewWatchlist::~CViewWatchlist()
{
	CMarketQuoteService::InstanceRef().SetQuoteHandler({ });
	for (const auto& item : s_watchlist)
	{
		CMarketQuoteService::InstanceRef().UnsubscribeQuote(item.m_pSecurity);
	}
	delete ui;
}

void CViewWatchlist::InitializeUI()
{
	ui->watchlistTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->depthTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	QStringList strLevels;
	for (int nLevel = 5; 1 <= nLevel; --nLevel)
	{
		strLevels.emplace_back(QString("卖%1").arg(nLevel));
	}
	for (int nLevel = 1; 5 >= nLevel; ++nLevel)
	{
		strLevels.emplace_back(QString("买%1").arg(nLevel));
	}
	for (int nRow = 0; nRow < strLevels.size(); ++nRow)
	{
		ui->depthTable->setItem(nRow, 0, new QTableWidgetItem(strLevels[nRow]));
		ui->depthTable->setItem(nRow, 1, new QTableWidgetItem("--"));
		ui->depthTable->setItem(nRow, 2, new QTableWidgetItem("--"));
	}
	connect(ui->watchlistTable, &QTableWidget::currentCellChanged, this, &CViewWatchlist::OnCurrentRowChanged);
	connect(ui->searchEdit, &QLineEdit::textChanged, this, &CViewWatchlist::OnFilterChanged);
}

void CViewWatchlist::InitializeWatchlist()
{
	ui->watchlistTable->setRowCount(static_cast<int>(s_watchlist.size()));
	for (int nRow = 0; nRow < static_cast<int>(s_watchlist.size()); ++nRow)
	{
		const CWatchlistItem& item = s_watchlist[static_cast<std::size_t>(nRow)];
		ui->watchlistTable->setItem(nRow, 0, new QTableWidgetItem(item.m_pSecurity));
		ui->watchlistTable->setItem(nRow, 1, new QTableWidgetItem(item.m_pName));
		for (int nColumn = 2; 6 > nColumn; ++nColumn)
		{
			ui->watchlistTable->setItem(nRow, nColumn, new QTableWidgetItem("--"));
		}
		ui->watchlistTable->setItem(nRow, 6, new QTableWidgetItem("订阅中"));
	}
	ui->watchlistTable->selectRow(0);
}

void CViewWatchlist::BindService()
{
	CMarketQuoteService& service = CMarketQuoteService::InstanceRef();
	service.Initialize();
	QPointer<CViewWatchlist> safeThis(this);
	service.SetQuoteHandler([safeThis](const CMarketQuoteSnapshot& snapshot)
	{
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, snapshot]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleQuote(snapshot);
			}
		}, Qt::QueuedConnection);
	});
	for (const auto& item : s_watchlist)
	{
		service.SubscribeQuote(item.m_pSecurity);
	}
}

void CViewWatchlist::HandleQuote(const CMarketQuoteSnapshot& snapshot)
{
	int nRow = FindSecurityRow(QString::fromStdString(snapshot.m_strSecurity));
	if (0 > nRow)
	{
		return;
	}
	double fChange = snapshot.m_fLastPrice - snapshot.m_fPreClose;
	double fPercent = 0.0 != snapshot.m_fPreClose ? fChange * 100.0 / snapshot.m_fPreClose : 0.0;
	ui->watchlistTable->item(nRow, 2)->setText(QString::number(snapshot.m_fLastPrice, 'f', 2));
	ui->watchlistTable->item(nRow, 3)->setText(QString("%1%2").arg(0.0 <= fChange ? "+" : "").arg(fChange, 0, 'f', 2));
	ui->watchlistTable->item(nRow, 4)->setText(QString("%1%2%").arg(0.0 <= fPercent ? "+" : "").arg(fPercent, 0, 'f', 2));
	ui->watchlistTable->item(nRow, 5)->setText(QString::number(snapshot.m_nVolume));
	ui->watchlistTable->item(nRow, 6)->setText(snapshot.m_bStale ? "已延迟" : "交易中");
	QColor color = 0.0 <= fChange ? QColor("#f04455") : QColor("#00b987");
	for (int nColumn = 2; 5 > nColumn; ++nColumn)
	{
		ui->watchlistTable->item(nRow, nColumn)->setForeground(color);
	}
}

int CViewWatchlist::FindSecurityRow(const QString& strSecurity) const
{
	QString strSymbol = strSecurity.section('.', 0, 0);
	for (int nRow = 0; nRow < ui->watchlistTable->rowCount(); ++nRow)
	{
		if (strSymbol == ui->watchlistTable->item(nRow, 0)->text().section('.', 0, 0))
		{
			return nRow;
		}
	}
	return -1;
}

void CViewWatchlist::OnCurrentRowChanged(int nCurrentRow, int, int, int)
{
	if (0 > nCurrentRow)
	{
		return;
	}
	QString strName = ui->watchlistTable->item(nCurrentRow, 1)->text();
	QString strSecurity = ui->watchlistTable->item(nCurrentRow, 0)->text();
	ui->chartTitle->setText(strName + "  " + strSecurity);
	ui->chartPlaceholder->setText("日 K 数据接口已预留\n等待 HQMarket 返回历史行情");
}

void CViewWatchlist::OnFilterChanged(const QString& strText)
{
	QString strFilter = strText.trimmed();
	for (int nRow = 0; nRow < ui->watchlistTable->rowCount(); ++nRow)
	{
		bool bMatched = strFilter.isEmpty() || ui->watchlistTable->item(nRow, 0)->text().contains(strFilter, Qt::CaseInsensitive) || ui->watchlistTable->item(nRow, 1)->text().contains(strFilter, Qt::CaseInsensitive);
		ui->watchlistTable->setRowHidden(nRow, !bMatched);
	}
}
