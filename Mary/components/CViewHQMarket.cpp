#include "CViewHQMarket.h"
#include "CUICurve.h"
#include "CUITable.h"
#include "ui_CViewHQMarket.h"

#include <QApplication>
#include <QButtonGroup>
#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cmath>

namespace
{
	class CDistributionChart final : public QWidget
	{
		public:
		explicit CDistributionChart(QWidget* parent) : QWidget(parent)
		{
			setMinimumHeight(110);
		}

		protected:
		void paintEvent(QPaintEvent*) override
		{
			QPainter painter(this);
			painter.setRenderHint(QPainter::Antialiasing);
			QRect area = rect().adjusted(28, 10, -8, -25);
			std::vector<int> values{ 80, 160, 260, 440, 730, 1100, 500, 200, 880, 500, 320, 190, 100 };
			painter.setPen(QPen(palette().color(QPalette::Mid), 0.5));
			for (int line = 0; 4 > line; ++line)
			{
				int y = area.bottom() - area.height() * line / 3;
				painter.drawLine(area.left(), y, area.right(), y);
				painter.drawText(QRect(0, y - 8, 25, 16), Qt::AlignRight, QString::number(line * 400));
			}
			double step = area.width() / 13.0;
			for (int index = 0; 13 > index; ++index)
			{
				painter.fillRect(QRectF(area.left() + index * step + 2, area.bottom() - values[index] * area.height() / 1200.0, step - 4, values[index] * area.height() / 1200.0), 6 > index ? QColor("#00b987") : QColor("#f04455"));
			}
			painter.setPen(palette().color(QPalette::Text));
			QStringList labels{ "<-5%", "-3%", "-1%", "0", "+1%", "+3%", ">5%" };
			for (int index = 0; 7 > index; ++index)
			{
				painter.drawText(QRectF(area.left() + index * area.width() / 7.0, area.bottom() + 5, area.width() / 7.0, 18), Qt::AlignCenter, labels[index]);
			}
		}
	};

	QFrame* Panel(QWidget* parent, QVBoxLayout*& layout)
	{
		QFrame* frame = new QFrame(parent);
		frame->setProperty("overviewPanel", true);
		layout = new QVBoxLayout(frame);
		layout->setContentsMargins(12, 10, 12, 10);
		layout->setSpacing(6);
		return frame;
	}

	QLabel* Heading(const QString& text, QWidget* parent)
	{
		QLabel* label = new QLabel(text, parent);
		label->setProperty("overviewHeading", true);
		label->setFixedHeight(18);
		return label;
	}

	QWidget* CreateOverview(QWidget* parent)
	{
		QWidget* page = new QWidget(parent);
		QVBoxLayout* root = new QVBoxLayout(page);
		root->setContentsMargins(14, 12, 14, 12);
		root->setSpacing(10);
		QHBoxLayout* indices = new QHBoxLayout();
		QStringList names{ "上证指数", "深证成指", "创业板指" };
		QStringList prices{ "3,428.16", "10,487.32", "2,176.45" };
		QStringList changes{ "+0.62%  +21.18", "+0.81%  +84.12", "-0.24%  -5.32" };
		for (int index = 0; 3 > index; ++index)
		{
			QVBoxLayout* layout = nullptr;
			QFrame* frame = Panel(page, layout);
			layout->addWidget(new QLabel(names[index], frame));
			QLabel* price = new QLabel(prices[index], frame);
			price->setStyleSheet(QString("font-size:23px;font-weight:600;color:%1;").arg(2 == index ? "#00b987" : "#f04455"));
			layout->addWidget(price);
			QLabel* change = new QLabel(changes[index], frame);
			change->setStyleSheet(QString("color:%1;").arg(2 == index ? "#00b987" : "#f04455"));
			layout->addWidget(change);
			indices->addWidget(frame);
		}
		root->addLayout(indices);
		QVBoxLayout* summaryLayout = nullptr;
		QFrame* summary = Panel(page, summaryLayout);
		summary->setFixedHeight(80);
		QHBoxLayout* summaryHeading = new QHBoxLayout();
		summaryHeading->setSpacing(12);
		summaryHeading->addWidget(Heading("市场概况", summary));
		QLabel* badge = new QLabel("示例数据", summary);
		badge->setProperty("overviewBadge", true);
		badge->setAlignment(Qt::AlignCenter);
		badge->setFixedSize(62, 18);
		summaryHeading->addWidget(badge);
		summaryHeading->addStretch();
		summaryLayout->addLayout(summaryHeading);
		QHBoxLayout* metrics = new QHBoxLayout();
		metrics->setSpacing(0);
		QStringList metricNames{ "沪深成交额", "上涨", "下跌", "平盘", "涨停", "跌停" };
		QStringList metricValues{ "1.26万亿", "3258", "1421", "186", "78", "12" };
		for (int index = 0; 6 > index; ++index)
		{
			QVBoxLayout* metric = new QVBoxLayout();
			metric->setSpacing(2);
			QLabel* name = new QLabel(metricNames[index], summary);
			name->setProperty("overviewMuted", true);
			name->setAlignment(Qt::AlignCenter);
			QLabel* value = new QLabel(metricValues[index], summary);
			value->setProperty("overviewValue", true);
			value->setProperty("rising", (1 == index) || (4 == index));
			value->setProperty("falling", (2 == index) || (5 == index));
			value->setAlignment(Qt::AlignCenter);
			metric->addWidget(name);
			metric->addWidget(value);
			metrics->addLayout(metric, 1);
			if (5 > index)
			{
				QFrame* separator = new QFrame(summary);
				separator->setProperty("overviewSeparator", true);
				separator->setFixedSize(1, 30);
				metrics->addWidget(separator);
			}
		}
		summaryLayout->addLayout(metrics);
		root->addWidget(summary);
		QHBoxLayout* analytics = new QHBoxLayout();
		QVBoxLayout* sectorLayout = nullptr;
		QFrame* sectors = Panel(page, sectorLayout);
		sectors->setFixedHeight(184);
		sectorLayout->addWidget(Heading("行业板块", sectors));
		QGridLayout* grid = new QGridLayout();
		grid->setSpacing(2);
		QButtonGroup* group = new QButtonGroup(page);
		QStringList sectorNames{ "银行", "食品饮料", "电子", "医药生物", "电力设备", "非银金融", "有色金属", "计算机", "汽车" };
		QStringList sectorChanges{ "+2.16%", "+1.32%", "+0.85%", "-0.41%", "-0.66%", "+1.26%", "+0.73%", "+0.56%", "-0.29%" };
		for (int index = 0; sectorNames.size() > index; ++index)
		{
			QPushButton* tile = new QPushButton(sectorNames[index] + "\n" + sectorChanges[index], sectors);
			tile->setCheckable(true);
			tile->setProperty("sectorTile", true);
			tile->setProperty("negative", sectorChanges[index].startsWith('-'));
			tile->setMinimumHeight(58);
			group->addButton(tile, index);
			grid->addWidget(tile, index / 5, index % 5);
		}
		sectorLayout->addLayout(grid, 1);
		analytics->addWidget(sectors, 42);
		QVBoxLayout* distributionLayout = nullptr;
		QFrame* distribution = Panel(page, distributionLayout);
		distribution->setFixedHeight(184);
		distributionLayout->addWidget(Heading("涨跌分布", distribution));
		distributionLayout->addWidget(new CDistributionChart(distribution), 1);
		analytics->addWidget(distribution, 29);
		QVBoxLayout* rankingLayout = nullptr;
		QFrame* ranking = Panel(page, rankingLayout);
		ranking->setFixedHeight(184);
		rankingLayout->addWidget(Heading("板块排行", ranking));
		QTableWidget* table = new QTableWidget(5, 3, ranking);
		table->setHorizontalHeaderLabels({ "序号", "行业名称", "涨跌幅" });
		table->verticalHeader()->hide();
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table->setSelectionBehavior(QAbstractItemView::SelectRows);
		table->setSelectionMode(QAbstractItemView::SingleSelection);
		table->setShowGrid(false);
		table->horizontalHeader()->setFixedHeight(24);
		table->verticalHeader()->setDefaultSectionSize(22);
		table->verticalHeader()->setMinimumSectionSize(22);
		table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		std::vector<int> order{ 0, 1, 5, 2, 6 };
		for (int row = 0; 5 > row; ++row)
		{
			table->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
			table->setItem(row, 1, new QTableWidgetItem(sectorNames[order[row]]));
			QTableWidgetItem* change = new QTableWidgetItem(sectorChanges[order[row]]);
			change->setForeground(QColor("#f04455"));
			table->setItem(row, 2, change);
		}
		rankingLayout->addWidget(table);
		analytics->addWidget(ranking, 27);
		root->addLayout(analytics);
		CUITable* constituents = new CUITable(MarketTableMode::Constituents, page);
		root->addWidget(constituents, 1);
		QObject::connect(group, &QButtonGroup::idClicked, page, [constituents, sectorNames, table, order](int index)
						 {
							 constituents->SetSector(sectorNames[index]);
							 table->clearSelection();
							 for (int row = 0; 5 > row; ++row)
							 {
								 if (index == order[row])
								 {
									 table->selectRow(row);
								 }
							 }
						 });
		QObject::connect(table, &QTableWidget::cellClicked, page, [constituents, sectorNames, group, order](int row, int)
						 {
							 group->button(order[row])->setChecked(true);
							 constituents->SetSector(sectorNames[order[row]]);
						 });
		group->button(0)->setChecked(true);
		table->selectRow(0);
		constituents->SetSector("银行");
		return page;
	}
} // namespace

CViewHQMarket::CViewHQMarket(QWidget* parent) : QWidget(parent), m_ui(std::make_unique<Ui::CViewHQMarketClass>())
{
	m_ui->setupUi(this);
	for (const QString& title : { QString("概览"), QString("自选"), QString("指数"), QString("A股"), QString("港股"), QString("美股") })
	{
		m_ui->marketTabBar->addTab(title);
	}
	m_ui->marketStack->addWidget(CreateOverview(m_ui->marketStack));
	for (int index = 1; 6 > index; ++index)
	{
		if ((1 == index) || (3 == index))
		{
			QWidget* page = new QWidget(m_ui->marketStack);
			QVBoxLayout* layout = new QVBoxLayout(page);
			layout->setContentsMargins(14, 12, 14, 0);
			layout->addWidget(new CUITable(1 == index ? MarketTableMode::Watchlist : MarketTableMode::AShare, page));
			m_ui->marketStack->addWidget(page);
		}
		else
		{
			QLabel* placeholder = new QLabel("该市场数据页将在后续阶段接入", m_ui->marketStack);
			placeholder->setAlignment(Qt::AlignCenter);
			m_ui->marketStack->addWidget(placeholder);
		}
	}
	connect(m_ui->marketTabBar, &QTabBar::currentChanged, m_ui->marketStack, &QStackedWidget::setCurrentIndex);
	ApplyTheme();
}

CViewHQMarket::~CViewHQMarket()
{
}

void CViewHQMarket::changeEvent(QEvent* event)
{
	QWidget::changeEvent(event);
	if (QEvent::PaletteChange == event->type())
	{
		ApplyTheme();
	}
}

void CViewHQMarket::ApplyTheme()
{
	bool dark = 128 > qApp->palette().color(QPalette::Window).lightness();
	QString typography = QString("QLabel[overviewHeading=\"true\"] {font-size:13px;font-weight:600;color:%1;} QLabel[overviewMuted=\"true\"] {font-size:12px;color:%2;} QLabel[overviewValue=\"true\"] {font-size:16px;font-weight:600;color:%1;} QLabel[overviewValue=\"true\"][rising=\"true\"] {color:#f04455;} QLabel[overviewValue=\"true\"][falling=\"true\"] {color:#00b987;} QLabel[overviewBadge=\"true\"] {font-size:11px;color:#3686ff;background:%3;border-radius:5px;} QFrame[overviewSeparator=\"true\"] {background:%4;border:0;}").arg(dark ? "#c8d8e7" : "#344258", dark ? "#8c9db6" : "#68778c", dark ? "#163454" : "#edf5fd", dark ? "#24334a" : "#e4eaf2");
	setStyleSheet(typography + QString("QFrame[overviewPanel=\"true\"] {background:%1;border:1px solid %2;border-radius:5px;} QPushButton[sectorTile=\"true\"] {background:%3;color:#f04455;border:1px solid transparent;border-radius:2px;} QPushButton[sectorTile=\"true\"][negative=\"true\"] {background:%4;color:#00b987;} QPushButton[sectorTile=\"true\"]:checked {border:2px solid #3686ff;}").arg(dark ? "#131f32" : "#ffffff", dark ? "#24334a" : "#e4eaf2", dark ? "#342332" : "#ffe0e4", dark ? "#123b3d" : "#d4f3e9"));
}
