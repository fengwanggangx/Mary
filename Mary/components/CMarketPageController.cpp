#include "CMarketPageController.h"
#include "CUITable.h"
#include "../system/CSession.h"
#include "CDataTableModel.h"
#include "CUICurve.h"

#include <QDateTime>
#include <QLabel>
#include <QMetaObject>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <cmath>

namespace
{
	struct CDemoSecurity
	{
			const char* m_code;
			const char* m_name;
			const char* m_sector;
			double m_price;
			double m_percent;
	};

	const std::vector<CDemoSecurity> DemoSecurities{
		{ "600519.SSE", "贵州茅台", "食品饮料", 1736.50, 1.23 },
		{ "300750.SZSE", "宁德时代", "电力设备", 231.20, 0.74 },
		{ "000858.SZSE", "五粮液", "食品饮料", 157.80, 1.09 },
		{ "601318.SSE", "中国平安", "非银金融", 48.52, -0.21 },
		{ "600036.SSE", "招商银行", "银行", 36.28, 2.16 },
		{ "688981.SSE", "中芯国际", "电子", 49.36, -1.20 },
		{ "002594.SZSE", "比亚迪", "汽车", 245.30, 1.45 },
		{ "300059.SZSE", "东方财富", "非银金融", 17.28, 0.93 },
		{ "601398.SSE", "工商银行", "银行", 6.23, 1.14 },
		{ "600900.SSE", "长江电力", "电力设备", 28.62, 0.35 },
		{ "002475.SZSE", "立讯精密", "电子", 35.42, -0.56 },
		{ "601012.SSE", "隆基绿能", "电力设备", 19.63, 0.82 },
		{ "601939.SSE", "建设银行", "银行", 8.42, 1.32 },
		{ "601288.SSE", "农业银行", "银行", 4.86, 1.25 },
		{ "601988.SSE", "中国银行", "银行", 5.23, 0.96 },
		{ "601328.SSE", "交通银行", "银行", 7.15, 1.42 },
		{ "000001.SZSE", "平安银行", "银行", 11.62, -0.34 },
		{ "600000.SSE", "浦发银行", "银行", 10.38, 1.07 },
		{ "601166.SSE", "兴业银行", "银行", 20.56, 1.68 },
		{ "600276.SSE", "恒瑞医药", "医药生物", 46.82, -0.41 },
		{ "600030.SSE", "中信证券", "非银金融", 27.10, 1.26 },
		{ "600050.SSE", "中国联通", "计算机", 5.12, 0.56 },
		{ "600111.SSE", "北方稀土", "有色金属", 22.46, 0.73 },
		{ "920001.BSE", "纬达光电", "电子", 12.30, 0.48 }
	};

	QString SectorOf(const QString& strCode)
	{
		for (const auto& demoValue : DemoSecurities)
		{
			if (strCode == QString::fromUtf8(demoValue.m_code))
			{
				return QString::fromUtf8(demoValue.m_sector);
			}
		}
		return QString();
	}

	class CMarketTableDelegate final : public QStyledItemDelegate
	{
		public:
			explicit CMarketTableDelegate(QObject* parent) : QStyledItemDelegate(parent)
			{
			}

		protected:
			void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
			{
				QStyledItemDelegate::initStyleOption(option, index);
				int nColumn = index.column();
				if (0 == nColumn)
				{
					option->text = index.data().toString().section('.', 0, 0);
				}
				if ((2 <= nColumn) && (5 >= nColumn))
				{
					double fValue = index.data().toDouble();
					option->text = QString::number(fValue, 'f', 2);
					if (4 == nColumn)
					{
						option->text += "%";
					}
					if (((3 == nColumn) || (4 == nColumn)) && (0.0 < fValue))
					{
						option->text.prepend('+');
					}
				}
				option->displayAlignment = 2 <= nColumn ? Qt::AlignRight | Qt::AlignVCenter : Qt::AlignLeft | Qt::AlignVCenter;
				if ((2 == nColumn) || (3 == nColumn) || (4 == nColumn))
				{
					double fPercent = index.siblingAtColumn(4).data().toDouble();
					QColor color = 0.0 <= fPercent ? QColor("#f04455") : QColor("#00b987");
					option->palette.setColor(QPalette::Text, color);
					option->palette.setColor(QPalette::HighlightedText, color);
				}
			}
	};
} // namespace

class CMarketFilterProxyModel final : public QSortFilterProxyModel
{
	public:
		explicit CMarketFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
		{
		}
		QString m_sector;
		int m_market{ -1 };
		const std::unordered_set<std::string>* m_watchlist{ nullptr };
		void Refresh()
		{
			invalidateFilter();
		}

	protected:
		bool filterAcceptsRow(int nRow, const QModelIndex& parent) const override
		{
			QString strCode = sourceModel()->index(nRow, 0, parent).data().toString();
			if ((nullptr != m_watchlist) && !m_watchlist->contains(strCode.toStdString()))
			{
				return false;
			}
			if (!m_sector.isEmpty() && (m_sector != SectorOf(strCode)))
			{
				return false;
			}
			QString strMarket = sourceModel()->index(nRow, 10, parent).data().toString();
			if (0 == m_market)
			{
				return ("沪A" == strMarket) || ("深A" == strMarket) || ("创业板" == strMarket) || ("科创板" == strMarket);
			}
			return (0 > m_market) || (1 == m_market && "北交所" == strMarket) || (2 == m_market && "创业板" == strMarket) || (3 == m_market && "科创板" == strMarket);
		}
};

CMarketPageController::CMarketPageController(MarketTableMode mode, CUITable* pTable, QObject* pParent)
	: QObject(pParent), m_mode(mode), m_table(pTable)
{
	m_demo = !CSession::InstanceRef().IsAuthenticated() || (MarketTableMode::Constituents == mode);
	m_model = new CDataTableModel(this);
	m_proxy = new CMarketFilterProxyModel(this);
	m_proxy->setSourceModel(m_model);
	if (MarketTableMode::Watchlist == mode)
	{
		m_proxy->m_watchlist = &m_watchlist;
	}
	m_table->setModel(m_proxy);
	m_table->setItemDelegate(new CMarketTableDelegate(m_table));
	connect(m_table, &CUITable::RowSelected, this, [this]()
	{
		RefreshSelection();
	});
	connect(m_table, &CUITable::ResultsChanged, this, [this]()
	{
		EnsureSelection();
	});
	if (m_demo)
	{
		LoadDemoData();
	}
	BindService();
}

CMarketPageController::~CMarketPageController()
{
	m_table->setModel(nullptr);
	CHQMarketService& service = CHQMarketService::InstanceRef();
	if (0 != m_quoteTableToken)
	{
		service.RemoveQuoteTableHandler(m_quoteTableToken);
	}
	if (0 != m_historyToken)
	{
		service.RemoveHistoryHandler(m_historyToken);
	}
}

void CMarketPageController::SetCharts(QLabel* pTitle, QLabel* pPrice, QLabel* pState, CUICurve* pIntraday, CUICurve* pCandles)
{
	m_stockTitle = pTitle;
	m_price = pPrice;
	m_chartState = pState;
	m_intraday = pIntraday;
	m_candles = pCandles;
	m_intraday->SetMode(CurveMode::Intraday);
	RefreshSelection();
}

void CMarketPageController::ToggleWatchlist(const QString& strCode)
{
	for (int nRow = 0; m_model->rowCount() > nRow; ++nRow)
	{
		QString strKey = m_model->index(nRow, 0).data().toString();
		if ((strKey == strCode) || (strKey.section('.', 0, 0) == strCode))
		{
			if (m_watchlist.contains(strKey.toStdString()))
			{
				m_watchlist.erase(strKey.toStdString());
			}
			else
			{
				m_watchlist.emplace(strKey.toStdString());
			}
			m_proxy->Refresh();
			m_table->Update();
			break;
		}
	}
}

void CMarketPageController::LoadDemoData()
{
	std::vector<CDataColumnSchema> schema{
		{ 1, "代码", DataType::String }, { 2, "名称", DataType::String }, { 3, "最新价", DataType::Double }, { 4, "涨跌额", DataType::Double }, { 5, "涨跌幅", DataType::Double }, { 6, "昨收", DataType::Double }, { 7, "成交量", DataType::Int64 }, { 8, "状态", DataType::String }, { 9, "序列", DataType::UInt64 }, { 10, "证券状态", DataType::String }, { 11, "市场", DataType::String }
	};
	for (const auto& column : schema)
	{
		m_demoTable.AddColumn(column);
	}
	CDataTableWriter writer = m_demoTable.BeginWrite();
	_TyDataRowId nRowId = 1;
	for (const auto& demoValue : DemoSecurities)
	{
		std::string strCode(demoValue.m_code);
		std::string strMarket = strCode.ends_with(".BSE") ? "北交所" : strCode.starts_with("688") ? "科创板"
																   : strCode.starts_with("300")	  ? "创业板"
																   : strCode.ends_with(".SSE")	  ? "沪A"
																								  : "深A";
		double fPreClose = demoValue.m_price / (1.0 + demoValue.m_percent / 100.0);
		writer.AddRow(nRowId, { strCode, std::string(demoValue.m_name), demoValue.m_price, demoValue.m_price - fPreClose, demoValue.m_percent, fPreClose, std::int64_t(1200000 + nRowId * 37000), std::string("示例"), std::uint64_t(0), std::string("normal"), strMarket });
		if (12 >= nRowId)
		{
			m_watchlist.emplace(strCode);
		}
		++nRowId;
	}
	auto [view, changes] = writer.Commit();
	HandleQuoteTable(view, changes);
}

void CMarketPageController::BindService()
{
	if (m_demo)
	{
		return;
	}
	CHQMarketService& service = CHQMarketService::InstanceRef();
	service.Initialize();
	QPointer<CMarketPageController> safeThis(this);
	m_quoteTableToken = service.AddQuoteTableHandler([safeThis](const CDataTableView& view, const CDataChangeSet& changes)
	{
		if (safeThis.isNull())
		{
			return;
		}
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, view, changes]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleQuoteTable(view, changes);
			}
		}, Qt::QueuedConnection);
	});
	m_historyToken = service.AddHistoryHandler([safeThis](std::uint64_t nId, const std::string& strSecurity, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& strError)
	{
		if (safeThis.isNull())
		{
			return;
		}
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, nId, strSecurity, period, bars, strError]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleHistory(nId, strSecurity, period, bars, strError);
			}
		}, Qt::QueuedConnection);
	});
	HandleQuoteTable(service.GetQuoteTableView(), CDataChangeSet{});
	service.QuerySecurities();
}

void CMarketPageController::SetSector(const QString& strSector)
{
	m_sector = strSector;
	m_proxy->m_sector = strSector;
	m_proxy->Refresh();
	m_table->Update();
	EnsureSelection();
}

void CMarketPageController::SetMarket(int nIndex)
{
	m_proxy->m_market = nIndex;
	m_proxy->Refresh();
	m_table->Update();
	EnsureSelection();
}

void CMarketPageController::EnsureSelection()
{
	if (!m_table->currentIndex().isValid() && (0 < m_table->ResultCount()))
	{
		m_table->selectRow(0);
	}
	RefreshSelection();
}

CSecurity CMarketPageController::GetSecurity(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return CSecurity();
	}
	CSecurity security = ParseSecurity(index.siblingAtColumn(0).data().toString().toStdString());
	security.m_strName = index.siblingAtColumn(1).data().toString().toStdString();
	return security;
}

void CMarketPageController::HandleQuoteTable(const CDataTableView& view, const CDataChangeSet& changes)
{
	m_model->SetView(view, changes);
	for (int nColumn = 0; m_model->columnCount() > nColumn; ++nColumn)
	{
		m_table->setColumnHidden(nColumn, MarketTableMode::Constituents == m_mode ? ((5 == nColumn) || (7 <= nColumn)) : ((3 == nColumn) || (5 <= nColumn)));
	}
	m_table->Update();
	EnsureSelection();
}

void CMarketPageController::RefreshSelection()
{
	if (nullptr == m_intraday)
	{
		return;
	}
	QModelIndex current = m_table->currentIndex();
	CSecurity security = GetSecurity(current);
	if (!security.IsValid())
	{
		m_stockTitle->setText("没有匹配的证券");
		m_selectedSecurity.clear();
		m_price->setText("--");
		m_chartState->clear();
		m_intraday->Clear();
		m_candles->Clear();
		m_minuteRequestId = 0;
		m_dayRequestId = 0;
		return;
	}
	m_stockTitle->setText(QString::fromStdString(security.m_strName + "  " + security.String()));
	double fPrice = current.siblingAtColumn(2).data().toDouble();
	double fPercent = current.siblingAtColumn(4).data().toDouble();
	m_price->setText(QString("%1    %2%3%").arg(fPrice, 0, 'f', 2).arg(0 <= fPercent ? "+" : "").arg(fPercent, 0, 'f', 2));
	m_price->setProperty("rising", 0 <= fPercent);
	m_price->style()->unpolish(m_price);
	m_price->style()->polish(m_price);
	if (m_selectedSecurity == security.String())
	{
		return;
	}
	m_selectedSecurity = security.String();
	m_chartState->setText(m_demo ? "演示行情 · 未连接服务器" : "正在查询历史行情");
	RequestHistory(CurveMode::Intraday);
	RequestHistory(m_candles->GetMode());
}

void CMarketPageController::RequestHistory(CurveMode mode)
{
	CSecurity security = GetSecurity(m_table->currentIndex());
	if (!security.IsValid())
	{
		return;
	}
	bool bMinute = CurveMode::Intraday == mode;
	CUICurve* curve = bMinute ? m_intraday : m_candles;
	curve->SetMode(mode);
	if (m_demo)
	{
		std::vector<CMarketBar> bars;
		int nCount = bMinute ? 120 : 90;
		bars.reserve(nCount);
		double fPrice = m_table->currentIndex().siblingAtColumn(2).data().toDouble();
		std::int64_t nEnd = QDateTime::currentMSecsSinceEpoch();
		for (int nIndex = 0; nCount > nIndex; ++nIndex)
		{
			double fClose = fPrice * (0.94 + 0.06 * nIndex / nCount + 0.008 * std::sin(nIndex * 0.4));
			double fOpen = fClose * (1.0 + 0.003 * std::sin(nIndex * 1.3));
			bars.emplace_back(CMarketBar{ nEnd - (nCount - nIndex) * std::int64_t(bMinute ? 60000 : 86400000), fOpen, (std::max)(fOpen, fClose) * 1.004, (std::min)(fOpen, fClose) * 0.996, fClose, std::int64_t(10000 + nIndex * 127), std::int64_t(0) });
		}
		curve->SetBars(bars);
		return;
	}
	curve->Clear();
	std::uint64_t& nId = bMinute ? m_minuteRequestId : m_dayRequestId;
	nId = 0;
	QDateTime now = QDateTime::currentDateTime();
	std::int64_t nBegin = bMinute ? QDateTime(QDate::currentDate(), QTime(0, 0)).toMSecsSinceEpoch() : now.addYears(-10).toMSecsSinceEpoch();
	if (!CHQMarketService::InstanceRef().QueryHistory(security, bMinute ? MarketBarPeriod::Minute : MarketBarPeriod::Day, nBegin, now.toMSecsSinceEpoch(), &nId))
	{
		m_chartState->setText("历史查询未发送，请检查连接");
	}
}

void CMarketPageController::HandleHistory(std::uint64_t nId, const std::string& strSecurity, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& strError)
{
	if (strSecurity != GetSecurity(m_table->currentIndex()).String())
	{
		return;
	}
	bool bMinute = MarketBarPeriod::Minute == period;
	if ((0 == nId) || (nId != (bMinute ? m_minuteRequestId : m_dayRequestId)))
	{
		return;
	}
	CUICurve* curve = bMinute ? m_intraday : m_candles;
	if (!strError.empty())
	{
		curve->Clear();
		m_chartState->setText(QString::fromStdString(strError));
		return;
	}
	curve->SetBars(bars);
	m_chartState->setText(bars.empty() ? "暂无历史行情" : "历史行情已加载");
}
