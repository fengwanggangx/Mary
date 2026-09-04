#include "CViewStrategySettings.h"
#include "ui_CViewStrategySettings.h"

CViewStrategySettings::CViewStrategySettings(QWidget* pParent) : QWidget(pParent), ui(new Ui::CViewStrategySettingsClass())
{
	ui->setupUi(this);
}

CViewStrategySettings::~CViewStrategySettings()
{
	delete ui;
}
