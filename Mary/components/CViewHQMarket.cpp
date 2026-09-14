#include "CViewHQMarket.h"
#include "CViewWatchlist.h"
#include "ui_CViewHQMarket.h"

#include <QLabel>

CViewHQMarket::CViewHQMarket(QWidget* pParent) : QWidget(pParent), ui(new Ui::CViewHQMarketClass())
{
	ui->setupUi(this);
	ui->marketTabBar->addTab("自选");
	ui->marketTabBar->addTab("指数");
	ui->marketTabBar->addTab("A股");
	ui->marketTabBar->addTab("港股");
	ui->marketTabBar->addTab("美股");
	ui->marketStack->addWidget(new CViewWatchlist(ui->marketStack));
	for (int nIndex = 1; 5 > nIndex; ++nIndex)
	{
		QLabel* pPlaceholder = new QLabel("该市场数据页将在后续阶段接入", ui->marketStack);
		pPlaceholder->setAlignment(Qt::AlignCenter);
		pPlaceholder->setObjectName("marketPlaceholder");
		ui->marketStack->addWidget(pPlaceholder);
	}
	connect(ui->marketTabBar, &QTabBar::currentChanged, ui->marketStack, &QStackedWidget::setCurrentIndex);
}

CViewHQMarket::~CViewHQMarket()
{
	delete ui;
}
