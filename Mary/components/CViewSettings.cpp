#include "CViewSettings.h"

CViewSettings::CViewSettings(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::CViewSettingsClass())
{
	ui->setupUi(this);
}

CViewSettings::~CViewSettings()
{
	delete ui;
}
