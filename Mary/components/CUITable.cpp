#include "CUITable.h"
#include "../system/CSession.h"
#include "CDataTableModel.h"
#include "CUICurve.h"

#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
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

	QString SectorOf(const QString& code)
	{
		for (const auto& value : DemoSecurities)
		{
			if (code == QString::fromUtf8(value.m_code))
			{
				return QString::fromUtf8(value.m_sector);
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
			int column = index.column();
			if (0 == column)
			{
				option->text = index.data().toString().section('.', 0, 0);
			}
			if ((2 <= column) && (5 >= column))
			{
				double value = index.data().toDouble();
				option->text = QString::number(value, 'f', 2);
				if (4 == column)
				{
					option->text += "%";
				}
				if (((3 == column) || (4 == column)) && (0.0 < value))
				{
					option->text.prepend('+');
				}
			}
			option->displayAlignment = 2 <= column ? Qt::AlignRight | Qt::AlignVCenter : Qt::AlignLeft | Qt::AlignVCenter;
			if ((2 == column) || (3 == column) || (4 == column))
			{
				double percent = index.siblingAtColumn(4).data().toDouble();
				QColor color = 0.0 <= percent ? QColor("#f04455") : QColor("#00b987");
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
	QString m_search;
	QString m_sector;
	int m_market{ -1 };
	const std::unordered_set<std::string>* m_watchlist{ nullptr };
	void Refresh()
	{
		invalidateFilter();
	}

	protected:
	bool filterAcceptsRow(int row, const QModelIndex& parent) const override
	{
		QString code = sourceModel()->index(row, 0, parent).data().toString();
		QString name = sourceModel()->index(row, 1, parent).data().toString();
		if (!m_search.isEmpty() && !code.contains(m_search, Qt::CaseInsensitive) && !name.contains(m_search, Qt::CaseInsensitive))
		{
			return false;
		}
		if ((nullptr != m_watchlist) && !m_watchlist->contains(code.toStdString()))
		{
			return false;
		}
		if (!m_sector.isEmpty() && (m_sector != SectorOf(code)))
		{
			return false;
		}
		QString market = sourceModel()->index(row, 10, parent).data().toString();
		if (0 == m_market)
		{
			return ("沪A" == market) || ("深A" == market) || ("创业板" == market) || ("科创板" == market);
		}
		return (0 > m_market) || (1 == m_market && "北交所" == market) || (2 == m_market && "创业板" == market) || (3 == m_market && "科创板" == market);
	}
};

CUITable::CUITable(QWidget* parent) : CUITable(MarketTableMode::Watchlist, parent)
{
}

CUITable::CUITable(MarketTableMode mode, QWidget* parent) : QWidget(parent), m_mode(mode)
{
	m_demo = !CSession::InstanceRef().IsAuthenticated() || (MarketTableMode::Constituents == mode);
	InitializeUI();
	if (m_demo)
	{
		LoadDemoData();
	}
	BindService();
	ApplyTheme();
}

CUITable::~CUITable()
{
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

void CUITable::InitializeUI()
{
	QVBoxLayout* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter = splitter;
	splitter->setHandleWidth(1);
	splitter->setChildrenCollapsible(false);
	root->addWidget(splitter);
	QFrame* list = new QFrame(splitter);
	list->setProperty("marketPanel", true);
	QVBoxLayout* layout = new QVBoxLayout(list);
	bool compact = MarketTableMode::Constituents != m_mode;
	layout->setContentsMargins(compact ? 0 : 12, compact ? 0 : 12, compact ? 0 : 12, 0);
	if (compact)
	{
		layout->setSpacing(0);
	}
	QHBoxLayout* heading = new QHBoxLayout();
	if (compact)
	{
		heading->setContentsMargins(12, 10, 12, 8);
	}
	m_title = new QLabel(MarketTableMode::AShare == m_mode ? "沪深A股" : MarketTableMode::Constituents == m_mode ? "成分股"
																												 : "自选股",
						 list);
	m_title->setProperty("marketHeading", true);
	m_count = new QLabel(list);
	heading->addWidget(m_title);
	heading->addSpacing(12);
	heading->addWidget(m_count);
	heading->addStretch();
	if (MarketTableMode::Watchlist == m_mode)
	{
		QPushButton* add = new QPushButton("＋", list);
		add->setToolTip("添加或移除自选证券");
		add->setFixedWidth(30);
		heading->addWidget(add);
		connect(add, &QPushButton::clicked, this, [this]()
				{
					bool accepted = false;
					QString code = QInputDialog::getText(this, "自选股", "输入证券代码（已在自选中则移除）", QLineEdit::Normal, QString(), &accepted).trimmed();
					if (!accepted || code.isEmpty())
					{
						return;
					}
					for (int row = 0; m_model->rowCount() > row; ++row)
					{
						QString key = m_model->index(row, 0).data().toString();
						if ((key == code) || (key.section('.', 0, 0) == code))
						{
							if (m_watchlist.contains(key.toStdString()))
							{
								m_watchlist.erase(key.toStdString());
							}
							else
							{
								m_watchlist.emplace(key.toStdString());
							}
							m_proxy->Refresh();
							UpdateCount();
							EnsureSelection();
							break;
						}
					}
				});
	}
	layout->addLayout(heading);
	QLineEdit* search = new QLineEdit(list);
	search->setPlaceholderText("搜索代码 / 名称");
	search->setClearButtonEnabled(true);
	search->setFixedWidth(180);
	search->setFixedHeight(32);
	heading->addWidget(search);
	heading->addSpacing(6);
	QPushButton* searchButton = new QPushButton(QString::fromUtf8("\xF0\x9F\x94\x8D"), list);
	searchButton->setObjectName("marketSearchButton");
	searchButton->setToolTip("搜索");
	searchButton->setAccessibleName("搜索");
	searchButton->setFixedSize(32, search->height());
	heading->addWidget(searchButton);
	connect(searchButton, &QPushButton::clicked, search, &QLineEdit::returnPressed);
	m_table = new QTableView(list);
	m_model = new CDataTableModel(m_table);
	m_proxy = new CMarketFilterProxyModel(m_table);
	m_proxy->setSourceModel(m_model);
	if (MarketTableMode::Watchlist == m_mode)
	{
		m_proxy->m_watchlist = &m_watchlist;
	}
	m_table->setModel(m_proxy);
	m_table->setItemDelegate(new CMarketTableDelegate(m_table));
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSortingEnabled(true);
	m_table->sortByColumn(-1, Qt::AscendingOrder);
	m_table->setShowGrid(false);
	if (compact)
	{
		m_table->setFrameShape(QFrame::NoFrame);
	}
	m_table->verticalHeader()->hide();
	m_table->verticalHeader()->setDefaultSectionSize(32);
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	layout->addWidget(m_table, 1);
	connect(search, &QLineEdit::textChanged, this, [this](const QString& text)
			{
				m_proxy->m_search = text.trimmed();
				m_proxy->Refresh();
				UpdateCount();
				EnsureSelection();
			});
	connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this]()
			{
				RefreshSelection();
			});
	connect(search, &QLineEdit::returnPressed, this, [this, search]()
			{
				m_proxy->m_search = search->text().trimmed();
				m_proxy->Refresh();
				UpdateCount();
				EnsureSelection();
			});
	if (MarketTableMode::AShare == m_mode)
	{
		m_marketTabs = new QTabBar(list);
		m_marketTabs->setObjectName("compactMarketTabs");
		m_marketTabs->setExpanding(false);
		m_marketTabs->setFixedHeight(28);
		for (const QString& title : { QString("沪深A股"), QString("北京A股"), QString("创业板"), QString("科创板") })
		{
			m_marketTabs->addTab(title);
		}
		layout->addWidget(m_marketTabs);
		connect(m_marketTabs, &QTabBar::currentChanged, this, &CUITable::SetMarket);
		SetMarket(0);
	}
	if (MarketTableMode::Constituents == m_mode)
	{
		return;
	}
	QWidget* charts = new QWidget(splitter);
	QVBoxLayout* chartLayout = new QVBoxLayout(charts);
	chartLayout->setContentsMargins(0, 0, 0, 0);
	chartLayout->setSpacing(0);
	QSplitter* chartSplitter = new QSplitter(Qt::Vertical, charts);
	chartSplitter->setChildrenCollapsible(false);
	chartLayout->addWidget(chartSplitter);
	QFrame* intradayFrame = new QFrame(chartSplitter);
	intradayFrame->setProperty("marketPanel", true);
	QVBoxLayout* intradayLayout = new QVBoxLayout(intradayFrame);
	m_stockTitle = new QLabel("选择证券", intradayFrame);
	m_stockTitle->setProperty("marketHeading", true);
	m_price = new QLabel("--", intradayFrame);
	m_chartState = new QLabel(intradayFrame);
	m_chartState->setProperty("marketMuted", true);
	intradayLayout->addWidget(m_stockTitle);
	intradayLayout->addWidget(m_price);
	intradayLayout->addWidget(m_chartState);
	intradayLayout->addWidget(new QLabel("分时", intradayFrame));
	m_intraday = new CUICurve(intradayFrame);
	m_intraday->SetMode(CurveMode::Intraday);
	intradayLayout->addWidget(m_intraday, 1);
	QFrame* candleFrame = new QFrame(chartSplitter);
	candleFrame->setProperty("marketPanel", true);
	QVBoxLayout* candleLayout = new QVBoxLayout(candleFrame);
	m_periodTabs = new QTabBar(candleFrame);
	m_periodTabs->setExpanding(false);
	for (const QString& title : { QString("日K"), QString("周K"), QString("月K") })
	{
		m_periodTabs->addTab(title);
	}
	candleLayout->addWidget(m_periodTabs);
	m_candles = new CUICurve(candleFrame);
	candleLayout->addWidget(m_candles, 1);
	connect(m_periodTabs, &QTabBar::currentChanged, this, [this](int index)
			{
				RequestHistory(0 == index ? CurveMode::Day : 1 == index ? CurveMode::Week
																		: CurveMode::Month);
			});
	splitter->setSizes({ 550, 450 });
	splitter->setStretchFactor(0, 55);
	splitter->setStretchFactor(1, 45);
	chartSplitter->setSizes({ 420, 420 });
}

void CUITable::LoadDemoData()
{
	std::vector<CDataColumnSchema> schema{
		{ 1, "代码", DataType::String }, { 2, "名称", DataType::String }, { 3, "最新价", DataType::Double }, { 4, "涨跌额", DataType::Double }, { 5, "涨跌幅", DataType::Double }, { 6, "昨收", DataType::Double }, { 7, "成交量", DataType::Int64 }, { 8, "状态", DataType::String }, { 9, "序列", DataType::UInt64 }, { 10, "证券状态", DataType::String }, { 11, "市场", DataType::String }
	};
	for (const auto& column : schema)
	{
		m_demoTable.AddColumn(column);
	}
	CDataTableWriter writer = m_demoTable.BeginWrite();
	_TyDataRowId rowId = 1;
	for (const auto& value : DemoSecurities)
	{
		std::string code(value.m_code);
		std::string market = code.ends_with(".BSE") ? "北交所" : code.starts_with("688") ? "科创板"
															 : code.starts_with("300")	 ? "创业板"
															 : code.ends_with(".SSE")	 ? "沪A"
																						 : "深A";
		double preClose = value.m_price / (1.0 + value.m_percent / 100.0);
		writer.AddRow(rowId, { code, std::string(value.m_name), value.m_price, value.m_price - preClose, value.m_percent, preClose, std::int64_t(1200000 + rowId * 37000), std::string("示例"), std::uint64_t(0), std::string("normal"), market });
		if (12 >= rowId)
		{
			m_watchlist.emplace(code);
		}
		++rowId;
	}
	auto [view, changes] = writer.Commit();
	HandleQuoteTable(view, changes);
}

void CUITable::BindService()
{
	if (m_demo)
	{
		return;
	}
	CHQMarketService& service = CHQMarketService::InstanceRef();
	service.Initialize();
	QPointer<CUITable> safeThis(this);
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
																				   },
																				   Qt::QueuedConnection);
													 });
	m_historyToken = service.AddHistoryHandler([safeThis](std::uint64_t id, const std::string& security, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& error)
											   {
												   if (safeThis.isNull())
												   {
													   return;
												   }
												   QMetaObject::invokeMethod(safeThis.data(), [safeThis, id, security, period, bars, error]()
																			 {
																				 if (!safeThis.isNull())
																				 {
																					 safeThis->HandleHistory(id, security, period, bars, error);
																				 }
																			 },
																			 Qt::QueuedConnection);
											   });
	HandleQuoteTable(service.GetQuoteTableView(), CDataChangeSet{});
	service.QuerySecurities();
}

void CUITable::SetSector(const QString& sector)
{
	m_sector = sector;
	m_proxy->m_sector = sector;
	m_proxy->Refresh();
	UpdateCount();
	EnsureSelection();
}

void CUITable::SetMarket(int index)
{
	m_title->setText(m_marketTabs->tabText(index));
	m_proxy->m_market = index;
	m_proxy->Refresh();
	UpdateCount();
	EnsureSelection();
}

void CUITable::UpdateCount()
{
	m_count->setText((m_sector.isEmpty() ? QString() : m_sector + " · ") + QString("共%1只").arg(m_proxy->rowCount()));
}

void CUITable::EnsureSelection()
{
	if (!m_table->currentIndex().isValid() && (0 < m_proxy->rowCount()))
	{
		m_table->selectRow(0);
	}
	RefreshSelection();
}

CSecurity CUITable::GetSecurity(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return CSecurity();
	}
	CSecurity security = ParseSecurity(index.siblingAtColumn(0).data().toString().toStdString());
	security.m_strName = index.siblingAtColumn(1).data().toString().toStdString();
	return security;
}

void CUITable::HandleQuoteTable(const CDataTableView& view, const CDataChangeSet& changes)
{
	m_model->SetView(view, changes);
	for (int column = 0; m_model->columnCount() > column; ++column)
	{
		m_table->setColumnHidden(column, MarketTableMode::Constituents == m_mode ? ((5 == column) || (7 <= column)) : ((3 == column) || (5 <= column)));
	}
	UpdateCount();
	EnsureSelection();
}

void CUITable::RefreshSelection()
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
	double price = current.siblingAtColumn(2).data().toDouble();
	double percent = current.siblingAtColumn(4).data().toDouble();
	m_price->setText(QString("%1    %2%3%").arg(price, 0, 'f', 2).arg(0 <= percent ? "+" : "").arg(percent, 0, 'f', 2));
	m_price->setStyleSheet(QString("font-size:20px; font-weight:600; color:%1;").arg(0 <= percent ? "#f04455" : "#00b987"));
	if (m_selectedSecurity == security.String())
	{
		return;
	}
	m_selectedSecurity = security.String();
	m_chartState->setText(m_demo ? "演示行情 · 未连接服务器" : "正在查询历史行情");
	RequestHistory(CurveMode::Intraday);
	RequestHistory(m_candles->GetMode());
}

void CUITable::RequestHistory(CurveMode mode)
{
	CSecurity security = GetSecurity(m_table->currentIndex());
	if (!security.IsValid())
	{
		return;
	}
	bool minute = CurveMode::Intraday == mode;
	CUICurve* curve = minute ? m_intraday : m_candles;
	curve->SetMode(mode);
	if (m_demo)
	{
		std::vector<CMarketBar> bars;
		int count = minute ? 120 : 90;
		bars.reserve(count);
		double price = m_table->currentIndex().siblingAtColumn(2).data().toDouble();
		std::int64_t end = QDateTime::currentMSecsSinceEpoch();
		for (int index = 0; count > index; ++index)
		{
			double close = price * (0.94 + 0.06 * index / count + 0.008 * std::sin(index * 0.4));
			double open = close * (1.0 + 0.003 * std::sin(index * 1.3));
			bars.emplace_back(CMarketBar{ end - (count - index) * std::int64_t(minute ? 60000 : 86400000), open, (std::max)(open, close) * 1.004, (std::min)(open, close) * 0.996, close, std::int64_t(10000 + index * 127), std::int64_t(0) });
		}
		curve->SetBars(bars);
		return;
	}
	curve->Clear();
	std::uint64_t& id = minute ? m_minuteRequestId : m_dayRequestId;
	id = 0;
	QDateTime now = QDateTime::currentDateTime();
	std::int64_t begin = minute ? QDateTime(QDate::currentDate(), QTime(0, 0)).toMSecsSinceEpoch() : now.addYears(-10).toMSecsSinceEpoch();
	if (!CHQMarketService::InstanceRef().QueryHistory(security, minute ? MarketBarPeriod::Minute : MarketBarPeriod::Day, begin, now.toMSecsSinceEpoch(), &id))
	{
		m_chartState->setText("历史查询未发送，请检查连接");
	}
}

void CUITable::HandleHistory(std::uint64_t id, const std::string& security, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& error)
{
	if (security != GetSecurity(m_table->currentIndex()).String())
	{
		return;
	}
	bool minute = MarketBarPeriod::Minute == period;
	if ((0 == id) || (id != (minute ? m_minuteRequestId : m_dayRequestId)))
	{
		return;
	}
	CUICurve* curve = minute ? m_intraday : m_candles;
	if (!error.empty())
	{
		curve->Clear();
		m_chartState->setText(QString::fromStdString(error));
		return;
	}
	curve->SetBars(bars);
	m_chartState->setText(bars.empty() ? "暂无历史行情" : "历史行情已加载");
}

void CUITable::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	if (!m_layoutInitialized && (MarketTableMode::Constituents != m_mode))
	{
		m_layoutInitialized = true;
		QTimer::singleShot(0, this, [this]()
						   {
							   int width = m_splitter->width() - m_splitter->handleWidth();
							   m_splitter->setSizes({ width * 55 / 100, width * 45 / 100 });
						   });
	}
}

void CUITable::changeEvent(QEvent* event)
{
	QWidget::changeEvent(event);
	if (QEvent::PaletteChange == event->type())
	{
		ApplyTheme();
	}
}

void CUITable::ApplyTheme()
{
	bool dark = 128 > qApp->palette().color(QPalette::Window).lightness();
	setStyleSheet(QString("QFrame[marketPanel=\"true\"] {background:%1; border:1px solid %2; border-radius:0;} QLabel[marketHeading=\"true\"] {font-size:15px;font-weight:600;} QLabel[marketMuted=\"true\"] {color:%3;} QTabBar#compactMarketTabs {margin:0;padding:0;} QTabBar#compactMarketTabs::tab {min-width:65px;height:20px;padding:0 10px;} QSplitter::handle {background:%2;width:1px;height:8px;}").arg(dark ? "#131f32" : "#ffffff", dark ? "#24334a" : "#e4eaf2", dark ? "#8c9db6" : "#68778c"));
}
