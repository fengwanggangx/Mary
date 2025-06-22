#pragma once

#include <QWidget>
#include "ui_CViewSettings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CViewSettingsClass; };
QT_END_NAMESPACE

class CViewSettings : public QWidget
{
	Q_OBJECT

public:
	CViewSettings(QWidget *parent = nullptr);
	~CViewSettings();

private:
	Ui::CViewSettingsClass *ui;
};
