#include "CUITable.h"

#include "../service/hqmarket/CHQMarketService.h"
#include "CUICurve.h"
#include "CDataTableModel.h"
#include "ui_CUITable.h"

#include <QHeaderView>
#include <QDateTime>
#include <QComboBox>
#include <QLayout>
#include <QMetaObject>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTableWidgetItem>

#include <cmath>

class CMarketFilterProxyModel final : public QSortFilterProxyModel
{
public:
	explicit CMarketFilterProxyModel(QObject* pParent) : QSortFilterProxyModel(pParent)
	{
	}

	void SetSearchText(const QString& strText)
	{
		m_strSearchText = strText.trimmed();
		invalidateFilter();
	}

	void SetMarket(const QString& strMarket)
	{
		m_strMarket = strMarket;
		invalidateFilter();
	}

	void SetStatus(const QString& strStatus)
	{
		m_strStatus = strStatus;
		invalidateFilter();
	}

protected:
	bool filterAcceptsRow(int nSourceRow, const QModelIndex& sourceParent) const override
	{
		QAbstractItemModel* pModel = sourceModel();
		if (nullptr == pModel)
		{
			return false;
		}
		if (!m_strSearchText.isEmpty())
		{
			QString strSecurity = pModel->index(nSourceRow, 0, sourceParent).data().toString();
			QString strName = pModel->index(nSourceRow, 1, sourceParent).data().toString();
			if (!strSecurity.contains(m_strSearchText, Qt::CaseInsensitive) && !strName.contains(m_strSearchText, Qt::CaseInsensitive))
			{
				return false;
			}
		}
		if (!m_strStatus.isEmpty() && (m_strStatus != pModel->index(nSourceRow, 9, sourceParent).data().toString()))
		{
			return false;
		}
		return m_strMarket.isEmpty() || (m_strMarket == pModel->index(nSourceRow, 10, sourceParent).data().toString());
	}

private:
	QString m_strSearchText;
	QString m_strMarket;
	QString m_strStatus;
};

namespace
{
	class CMarketTableDelegate final : public QStyledItemDelegate
	{
	public:
		explicit CMarketTableDelegate(QObject* pParent) : QStyledItemDelegate(pParent)
		{
		}

	protected:
		void initStyleOption(QStyleOptionViewItem* pOption, const QModelIndex& index) const override
		{
			QStyledItemDelegate::initStyleOption(pOption, index);
			int nColumn = index.column();
			if ((2 <= nColumn) && (5 >= nColumn))
			{
				double fValue = index.data(Qt::DisplayRole).toDouble();
				pOption->text = 4 == nColumn ? QString::number(fValue, 'f', 2) + "%" : QString::number(fValue, 'f', 2);
			}
			else if (6 == nColumn)
			{
				pOption->text = QString::number(index.data(Qt::DisplayRole).toLongLong());
			}
			if ((3 == nColumn) || (4 == nColumn))
			{
				double fChange = index.data(Qt::DisplayRole).toDouble();
				if (0.0 < fChange)
				{
					pOption->palette.setColor(QPalette::Text, QColor(220, 55, 55));
				}
				else if (0.0 > fChange)
				{
					pOption->palette.setColor(QPalette::Text, QColor(25, 155, 85));
				}
			}
		}
	};

}

CUITable::CUITable(QWidget* pParent) : QWidget(pParent), ui(new Ui::CUITableClass())
{
	ui->setupUi(this);
	InitializeUI();
	BindService();
}

CUITable::~CUITable()
{
	CHQMarketService& service = CHQMarketService::InstanceRef();
	if (0 != m_quoteTableHandlerToken)
	{
		service.RemoveQuoteTableHandler(m_quoteTableHandlerToken);
	}
	if (0 != m_depthHandlerToken)
	{
		service.RemoveDepthHandler(m_depthHandlerToken);
	}
	if (0 != m_historyHandlerToken)
	{
		service.RemoveHistoryHandler(m_historyHandlerToken);
	}
	delete ui;
}

void CUITable::InitializeUI()
{
	m_pCurve = new CUICurve(ui->chartFrame);
	ui->chartPlaceholder->parentWidget()->layout()->replaceWidget(ui->chartPlaceholder, m_pCurve);
	ui->chartPlaceholder->deleteLater();
	m_pWatchlistTable = new QTableView(ui->watchlistTable->parentWidget());
	m_pWatchlistModel = new CDataTableModel(m_pWatchlistTable);
	m_pWatchlistProxy = new CMarketFilterProxyModel(m_pWatchlistTable);
	m_pWatchlistProxy->setSourceModel(m_pWatchlistModel);
	m_pWatchlistProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_pWatchlistProxy->setFilterKeyColumn(-1);
	m_pWatchlistProxy->setDynamicSortFilter(true);
	m_pWatchlistTable->setModel(m_pWatchlistProxy);
	m_pWatchlistTable->setItemDelegate(new CMarketTableDelegate(m_pWatchlistTable));
	m_pWatchlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pWatchlistTable->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pWatchlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pWatchlistTable->setSortingEnabled(true);
	m_pWatchlistTable->verticalHeader()->setVisible(false);
	m_pWatchlistTable->verticalHeader()->setDefaultSectionSize(24);
	ui->watchlistTable->parentWidget()->layout()->replaceWidget(ui->watchlistTable, m_pWatchlistTable);
	ui->watchlistTable->deleteLater();
	m_pWatchlistTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_pWatchlistTable->setColumnHidden(8, true);
	ui->marketCombo->addItem("全部市场", QString());
	ui->marketCombo->addItem("沪A", "沪A");
	ui->marketCombo->addItem("深A", "深A");
	ui->marketCombo->addItem("创业板", "创业板");
	ui->marketCombo->addItem("科创板", "科创板");
	ui->marketCombo->addItem("北交所", "北交所");
	ui->statusCombo->addItem("全部状态", QString());
	ui->statusCombo->addItem("正常", "normal");
	ui->statusCombo->addItem("停牌", "suspended");
	ui->statusCombo->addItem("退市", "delisted");
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
	connect(m_pWatchlistTable->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &CUITable::OnCurrentRowChanged);
	connect(ui->searchEdit, &QLineEdit::textChanged, this, &CUITable::OnFilterChanged);
	connect(ui->marketCombo, &QComboBox::currentIndexChanged, this, &CUITable::OnMarketFilterChanged);
	connect(ui->statusCombo, &QComboBox::currentIndexChanged, this, &CUITable::OnStatusFilterChanged);
	connect(ui->intradayButton, &QPushButton::clicked, this, [this]()
	{
		RequestHistory(CurveMode::Intraday);
	});
	connect(ui->dayButton, &QPushButton::clicked, this, [this]()
	{
		RequestHistory(CurveMode::Day);
	});
	connect(ui->weekButton, &QPushButton::clicked, this, [this]()
	{
		RequestHistory(CurveMode::Week);
	});
	connect(ui->monthButton, &QPushButton::clicked, this, [this]()
	{
		RequestHistory(CurveMode::Month);
	});
}

void CUITable::BindService()
{
	CHQMarketService& service = CHQMarketService::InstanceRef();
	service.Initialize();
	QPointer<CUITable> safeThis(this);
	m_pWatchlistModel->SetView(service.GetQuoteTableView(), CDataChangeSet{ });
	m_quoteTableHandlerToken = service.AddQuoteTableHandler([safeThis](const CDataTableView& view, const CDataChangeSet& changes)
	{
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, view, changes]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleQuoteTable(view, changes);
			}
		}, Qt::QueuedConnection);
	});
	m_depthHandlerToken = service.AddDepthHandler([safeThis](const CMarketDepth& depth)
	{
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, depth]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleDepth(depth);
			}
		}, Qt::QueuedConnection);
	});
	m_historyHandlerToken = service.AddHistoryHandler([safeThis](std::uint64_t requestId, const std::string& strSecurity, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& strError)
	{
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, requestId, strSecurity, period, bars, strError]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleHistory(requestId, strSecurity, period, bars, strError);
			}
		}, Qt::QueuedConnection);
	});
}

void CUITable::HandleDepth(const CMarketDepth& depth)
{
	if (!(GetSecurity(m_pWatchlistTable->currentIndex()) == depth.m_security))
	{
		return;
	}

	for (int nLevel = 0; 5 > nLevel; ++nLevel)
	{
		int nAskRow = 4 - nLevel;
		if (nLevel < static_cast<int>(depth.m_asks.size()))
		{
			const CPriceLevel& level = depth.m_asks[static_cast<std::size_t>(nLevel)];
			ui->depthTable->item(nAskRow, 1)->setText(QString::number(level.m_fPrice, 'f', 2));
			ui->depthTable->item(nAskRow, 2)->setText(QString::number(level.m_nVolume));
		}
		int nBidRow = 5 + nLevel;
		if (nLevel < static_cast<int>(depth.m_bids.size()))
		{
			const CPriceLevel& level = depth.m_bids[static_cast<std::size_t>(nLevel)];
			ui->depthTable->item(nBidRow, 1)->setText(QString::number(level.m_fPrice, 'f', 2));
			ui->depthTable->item(nBidRow, 2)->setText(QString::number(level.m_nVolume));
		}
	}
}

void CUITable::HandleHistory(std::uint64_t requestId, const std::string& strSecurity, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& strError)
{
	MarketBarPeriod expectedPeriod = CurveMode::Intraday == m_pCurve->GetMode() ? MarketBarPeriod::Minute : MarketBarPeriod::Day;
	if ((requestId != m_historyRequestId) || (expectedPeriod != period) || (GetSecurity(m_pWatchlistTable->currentIndex()).String() != strSecurity))
	{
		return;
	}
	if (!strError.empty())
	{
		m_pCurve->Clear();
		return;
	}
	m_pCurve->SetBars(bars);
}

void CUITable::HandleQuoteTable(const CDataTableView& view, const CDataChangeSet& changes)
{
	m_pWatchlistModel->SetView(view, changes);
	if (!m_pWatchlistTable->currentIndex().isValid() && (0 < m_pWatchlistProxy->rowCount()))
	{
		m_pWatchlistTable->selectRow(0);
	}
}

CSecurity CUITable::GetSecurity(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return { };
	}
	CSecurity security = ParseSecurity(m_pWatchlistProxy->index(index.row(), 0).data().toString().toStdString());
	security.m_strName = GetName(index).toStdString();
	return security;
}

QString CUITable::GetName(const QModelIndex& index) const
{
	return index.isValid() ? m_pWatchlistProxy->index(index.row(), 1).data().toString() : QString();
}

void CUITable::OnCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
	CSecurity security = GetSecurity(current);
	if (!security.IsValid())
	{
		return;
	}
	QString strName = GetName(current);
	ui->chartTitle->setText(strName + "  " + QString::fromStdString(security.String()));
	m_pCurve->Clear();
	CHQMarketService& service = CHQMarketService::InstanceRef();
	CSecurity previousSecurity = GetSecurity(previous);
	if (previousSecurity.IsValid())
	{
		service.UnsubscribeDepth(previousSecurity);
	}
	service.SubscribeDepth(security);
	RequestHistory(m_pCurve->GetMode());
}

void CUITable::OnFilterChanged(const QString& strText)
{
	m_pWatchlistProxy->SetSearchText(strText);
}

void CUITable::OnMarketFilterChanged(int nIndex)
{
	m_pWatchlistProxy->SetMarket(ui->marketCombo->itemData(nIndex).toString());
}

void CUITable::OnStatusFilterChanged(int nIndex)
{
	m_pWatchlistProxy->SetStatus(ui->statusCombo->itemData(nIndex).toString());
}

void CUITable::RequestHistory(CurveMode mode)
{
	m_historyRequestId = 0;
	CSecurity security = GetSecurity(m_pWatchlistTable->currentIndex());
	if (!security.IsValid())
	{
		return;
	}
	m_pCurve->SetMode(mode);
	m_pCurve->Clear();
	std::int64_t nEndTime = QDateTime::currentMSecsSinceEpoch();
	MarketBarPeriod period = CurveMode::Intraday == mode ? MarketBarPeriod::Minute : MarketBarPeriod::Day;
	std::int64_t nBeginTime = 0;
	if (MarketBarPeriod::Minute == period)
	{
		nBeginTime = QDateTime(QDate::currentDate(), QTime(0, 0)).toMSecsSinceEpoch();
	}
	else
	{
		nBeginTime = QDateTime::currentDateTime().addYears(-10).toMSecsSinceEpoch();
	}
	CHQMarketService::InstanceRef().QueryHistory(security, period, nBeginTime, nEndTime, &m_historyRequestId);
}
