#include "CRiskSettings.h"
#include "ui_CRiskSettings.h"

CRiskSettings::CRiskSettings(QWidget* pParent) : QWidget(pParent), ui(new Ui::CRiskSettingsClass())
{
	ui->setupUi(this);
}

CRiskSettings::~CRiskSettings()
{
	delete ui;
}
