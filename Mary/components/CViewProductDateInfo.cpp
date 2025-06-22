#include "CViewProductDateInfo.h"

CViewProductDateInfo::CViewProductDateInfo(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::CViewProductDateInfoClass())
{
	ui->setupUi(this);
}

CViewProductDateInfo::~CViewProductDateInfo()
{
	delete ui;
}
