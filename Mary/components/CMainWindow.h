#pragma once

#include <QMainWindow>
#include <QPoint>
#include <memory>

class QAction;
namespace Ui
{
	class CMainWindowClass;
}

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
		void UpdateWindowButtonIcons();
		void UpdateConnectionState(int nState, const QString& strMessage);
		void changeEvent(QEvent* pEvent) override;
		bool eventFilter(QObject* pObject, QEvent* pEvent) override;

		std::unique_ptr<Ui::CMainWindowClass> m_ui;
		QPoint m_dragPosition;
		bool m_bDarkTheme{ false };
		bool m_bDragging{ false };
};
