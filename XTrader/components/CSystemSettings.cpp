#include "CSystemSettings.h"
#include "ui_CSystemSettings.h"

CSystemSettings::CSystemSettings(QWidget* pParent) : QWidget(pParent), ui(new Ui::CSystemSettingsClass())
{
	ui->setupUi(this);
}

CSystemSettings::~CSystemSettings()
{
	delete ui;
}
