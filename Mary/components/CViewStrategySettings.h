#ifndef MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H
#define MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H

#include <QWidget>
#include <memory>

namespace Ui
{
	class CViewStrategySettingsClass;
}

class QLabel;
class QLineEdit;
class QComboBox;
class QTableWidget;

class CViewStrategySettings final : public QWidget
{
		Q_OBJECT
	public:
		explicit CViewStrategySettings(QWidget* pParent = nullptr);
		~CViewStrategySettings() override;

	private:
		void ApplyTheme();
		void changeEvent(QEvent* pEvent) override;
		std::unique_ptr<Ui::CViewStrategySettingsClass> m_ui;
		void RefreshFilter();
		void RefreshDetails();
		QTableWidget* m_pInstances{ nullptr };
		QLabel* m_pDetails{ nullptr };
		QLineEdit* m_pSearch{ nullptr };
		QComboBox* m_pStatus{ nullptr };
};
#endif
