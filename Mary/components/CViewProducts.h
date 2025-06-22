#pragma once

#include <QWidget>
#include "ui_CViewProducts.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CViewProductsClass; };
QT_END_NAMESPACE

class CViewProducts : public QWidget
{
	Q_OBJECT

public:
	CViewProducts(QWidget *parent = nullptr);
	~CViewProducts();

private:
	Ui::CViewProductsClass *ui;
};
