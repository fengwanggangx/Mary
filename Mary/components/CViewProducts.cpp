#include "CViewProducts.h"

CViewProducts::CViewProducts(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::CViewProductsClass())
{
	ui->setupUi(this);
}

CViewProducts::~CViewProducts()
{
	delete ui;
}
