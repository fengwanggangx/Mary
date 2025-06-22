#pragma once

#include <QWidget>
#include "ui_CViewProductDateInfo.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CViewProductDateInfoClass; };
QT_END_NAMESPACE

class CViewProductDateInfo : public QWidget
{
	Q_OBJECT

public:
	CViewProductDateInfo(QWidget *parent = nullptr);
	~CViewProductDateInfo();

private:
	Ui::CViewProductDateInfoClass *ui;
};
