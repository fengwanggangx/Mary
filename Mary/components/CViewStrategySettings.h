#ifndef MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H
#define MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QComboBox;
class QTableWidget;

class CViewStrategySettings final : public QWidget
{
	Q_OBJECT
public:
	explicit CViewStrategySettings(QWidget* pParent = nullptr);
	~CViewStrategySettings() override = default;
private:
	QWidget* CreateInstancePage();
	void RefreshFilter();
	void RefreshDetails();
	QTableWidget* m_pInstances{ nullptr };
	QLabel* m_pDetails{ nullptr };
	QLineEdit* m_pSearch{ nullptr };
	QComboBox* m_pStatus{ nullptr };
};
#endif
