#pragma once

#include <QMainWindow>
#include <QMap>
#include <QString>

#include <memory>

#include "ui_CMainWindow.h"

class CWindSession;
class QLabel;
class QListWidget;
class QPushButton;

class CMainWindow final : public QMainWindow
{
	Q_OBJECT

public:
	explicit CMainWindow(QWidget* pParent = nullptr);
	~CMainWindow() override;

private slots:
	void OnItemSelectChanged();
	void OnAuthenticated(bool authenticated);
	void OnResponse(qulonglong id, const QString& strCommand, const QMap<QString, QString>& result);
	void OnRequestFailed(qulonglong id, const QString& strCommand, const QString& strError);

private:
	void ConnectSlots();
	void UIInitialized();
	void BuildWindPage();
	void SetControlsEnabled(bool enabled);
	void SendStrategyCommand(const QString& strCommand);

	Ui::CMainWindowClass* ui{ nullptr };
	std::unique_ptr<CWindSession> m_windSession;
	QLabel* m_connectionLabel{ nullptr };
	QLabel* m_versionLabel{ nullptr };
	QLabel* m_serviceLabel{ nullptr };
	QLabel* m_riskLabel{ nullptr };
	QLabel* m_errorLabel{ nullptr };
	QListWidget* m_strategyList{ nullptr };
	QPushButton* m_startButton{ nullptr };
	QPushButton* m_stopButton{ nullptr };
	QPushButton* m_riskButton{ nullptr };
	bool m_riskEnabled{ false };
};
