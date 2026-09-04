#include "CViewHQMarket.h"
#include "ui_CViewHQMarket.h"

CViewHQMarket::CViewHQMarket(QWidget* pParent) : QWidget(pParent), ui(new Ui::CViewHQMarketClass())
{
	ui->setupUi(this);
}

CViewHQMarket::~CViewHQMarket()
{
	delete ui;
}
