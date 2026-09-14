#pragma once

#include "ui_CMainWindow.h"

#include <QMainWindow>
#include <QPoint>

class QAction;

class CMainWindow final : public QMainWindow
{
	Q_OBJECT

public:
	explicit CMainWindow(QWidget* pParent = nullptr);
	~CMainWindow() override;

private slots:
	void OnItemSelectChanged();
	void OnLightTheme();
	void OnDarkTheme();
	void UpdateClock();

private:
	void ConnectSlots();
	void UIInitialized();
	void ApplyTheme(bool bDark);
	void UpdateConnectionState(int nState, const QString& strMessage);
	bool eventFilter(QObject* pObject, QEvent* pEvent) override;

	Ui::CMainWindowClass* ui{ nullptr };
	QAction* m_pLightThemeAction{ nullptr };
	QAction* m_pDarkThemeAction{ nullptr };
	QPoint m_dragPosition;
	bool m_bDragging{ false };
};
