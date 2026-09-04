#pragma once

#include "ui_CMainWindow.h"

#include <QMainWindow>

class CMainWindow final : public QMainWindow
{
	Q_OBJECT

public:
	explicit CMainWindow(QWidget* pParent = nullptr);
	~CMainWindow() override;

private slots:
	void OnItemSelectChanged();

private:
	void ConnectSlots();
	void UIInitialized();

	Ui::CMainWindowClass* ui{nullptr};
};
