#include "CMainWindow.h"

#include "CRiskSettings.h"
#include "CSystemSettings.h"
#include "CViewHQMarket.h"
#include "CViewStrategySettings.h"

#include <QTreeWidgetItem>

#include <array>
#include <cstddef>

namespace
{
	struct CPageInfo
	{
		QString m_key;
		QString m_title;
		QString m_icon;
	};

	const std::vector<CPageInfo> s_pages
	{
		CPageInfo{ "hqmarket", "市场行情", ":/images/setting.png" },
		CPageInfo{ "strategy_settings", "策略管理", ":/images/setting.png" },
		CPageInfo{ "risk_settings", "风控设置", ":/images/setting.png" },
		CPageInfo{ "system_settings", "系统设置", ":/images/setting.png" }
	};

	QWidget* CreateView(const QString& key, QWidget* pParent)
	{
		if ("hqmarket" == key)
		{
			return new CViewHQMarket(pParent);
		}

		if ("strategy_settings" == key)
		{
			return new CViewStrategySettings(pParent);
		}

		if ("risk_settings" == key)
		{
			return new CRiskSettings(pParent);
		}

		if ("system_settings" == key)
		{
			return new CSystemSettings(pParent);
		}

		return nullptr;
	}

}

CMainWindow::CMainWindow(QWidget* pParent) : QMainWindow(pParent), ui(new Ui::CMainWindowClass())
{
	ui->setupUi(this);
	UIInitialized();
	ConnectSlots();
}

CMainWindow::~CMainWindow()
{
	delete ui;
}

void CMainWindow::UIInitialized()
{
	ui->treeWidget->clear();
	ui->treeWidget->setIndentation(10);

	std::size_t sz = s_pages.size();
	for (std::size_t idx = 0; idx < sz; ++idx)
	{
		const CPageInfo& info = s_pages[idx];
		ui->stackedWidget->addWidget(CreateView(info.m_key, ui->stackedWidget));
		QTreeWidgetItem* pItem = new QTreeWidgetItem(ui->treeWidget);
		pItem->setText(0, info.m_title);
		pItem->setIcon(0, QIcon(info.m_icon));
		pItem->setData(0, Qt::UserRole, static_cast<int>(idx));
	}

	QTreeWidgetItem* pFirstItem = ui->treeWidget->topLevelItem(0);
	if (nullptr != pFirstItem)
	{
		ui->treeWidget->setCurrentItem(pFirstItem);
		ui->stackedWidget->setCurrentIndex(0);
	}
}

void CMainWindow::ConnectSlots()
{
	connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &CMainWindow::OnItemSelectChanged);
}

void CMainWindow::OnItemSelectChanged()
{
	QTreeWidgetItem* pItem = ui->treeWidget->currentItem();
	if (nullptr == pItem)
	{
		return;
	}

	int index = pItem->data(0, Qt::UserRole).toInt();
	if (0 <= index && ui->stackedWidget->count() > index)
	{
		ui->stackedWidget->setCurrentIndex(index);
	}
}
