#include "CViewHQMarket.h"
#include "CUIStyle.h"
#include "ui_CViewHQMarket.h"

#include <QApplication>
#include <QButtonGroup>
#include <QPushButton>
#include <QEvent>
#include <QHeaderView>
#include <QTabBar>
#include <vector>

CViewHQMarket::CViewHQMarket(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CViewHQMarketClass>())
{
	m_ui->setupUi(this);
	m_ui->marketTabs->tabBar()->setDrawBase(false);
	m_ui->marketTabs->tabBar()->setExpanding(false);
	m_ui->shanghaiPrice->setText("3,428.16");
	m_ui->shenzhenPrice->setText("10,487.32");
	m_ui->chinextPrice->setText("2,176.45");
	m_ui->shanghaiChange->setText("+0.62%  +21.18");
	m_ui->shenzhenChange->setText("+0.81%  +84.12");
	m_ui->chinextChange->setText("-0.24%  -5.32");
	QList<QLabel*> metricValues{ m_ui->metric0Value, m_ui->metric1Value, m_ui->metric2Value, m_ui->metric3Value, m_ui->metric4Value, m_ui->metric5Value };
	QStringList values{ "1.26万亿", "3258", "1421", "186", "78", "12" };
	for (int nIndex = 0; metricValues.size() > nIndex; ++nIndex)
	{
		metricValues[nIndex]->setText(values[nIndex]);
	}
	m_ui->distributionChart->SetValues({ 80, 160, 260, 440, 730, 1100, 500, 200, 880, 500, 320, 190, 100 });
	QButtonGroup* group = new QButtonGroup(this);
	QStringList sectorNames{ "银行", "食品饮料", "电子", "医药生物", "电力设备", "非银金融", "有色金属", "计算机", "汽车" };
	QStringList sectorChanges{ "+2.16%", "+1.32%", "+0.85%", "-0.41%", "-0.66%", "+1.26%", "+0.73%", "+0.56%", "-0.29%" };
	for (int nIndex = 0; sectorNames.size() > nIndex; ++nIndex)
	{
		QPushButton* tile = new QPushButton(sectorNames[nIndex] + "\n" + sectorChanges[nIndex], m_ui->sectorsPanel);
		tile->setCheckable(true);
		tile->setProperty("sectorTile", true);
		tile->setProperty("negative", sectorChanges[nIndex].startsWith('-'));
		tile->setMinimumHeight(58);
		group->addButton(tile, nIndex);
		m_ui->sectorGrid->addWidget(tile, nIndex / 5, nIndex % 5);
	}
	std::vector<int> order{ 0, 1, 5, 2, 6 };
	m_ui->rankingTable->setRowCount(static_cast<int>(order.size()));
	m_ui->rankingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_ui->rankingTable->horizontalHeader()->setFixedHeight(24);
	for (int nRow = 0; order.size() > static_cast<std::size_t>(nRow); ++nRow)
	{
		m_ui->rankingTable->setItem(nRow, 0, new QTableWidgetItem(QString::number(nRow + 1)));
		m_ui->rankingTable->setItem(nRow, 1, new QTableWidgetItem(sectorNames[order[nRow]]));
		m_ui->rankingTable->setItem(nRow, 2, new QTableWidgetItem(sectorChanges[order[nRow]]));
	}
	connect(group, &QButtonGroup::idClicked, this, [this, sectorNames, order](int nIndex)
	{
		m_ui->constituentsPage->SetSector(sectorNames[nIndex]);
		m_ui->rankingTable->clearSelection();
		for (int nRow = 0; order.size() > static_cast<std::size_t>(nRow); ++nRow)
		{
			if (nIndex == order[nRow])
			{
				m_ui->rankingTable->selectRow(nRow);
			}
		}
	});
	connect(m_ui->rankingTable, &QTableWidget::cellClicked, this, [this, sectorNames, group, order](int nRow, int)
	{
		group->button(order[nRow])->setChecked(true);
		m_ui->constituentsPage->SetSector(sectorNames[order[nRow]]);
	});
	group->button(0)->setChecked(true);
	m_ui->rankingTable->selectRow(0);
	m_ui->constituentsPage->SetSector("银行");
	ApplyTheme();
}

CViewHQMarket::~CViewHQMarket() = default;

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
	m_ui->shanghaiPrice->ensurePolished();
	QColor risingColor = m_ui->shanghaiPrice->palette().color(QPalette::WindowText);
	for (int nRow = 0; m_ui->rankingTable->rowCount() > nRow; ++nRow)
	{
		QTableWidgetItem* change = m_ui->rankingTable->item(nRow, 2);
		if (nullptr != change)
		{
			change->setForeground(risingColor);
		}
	}
}
