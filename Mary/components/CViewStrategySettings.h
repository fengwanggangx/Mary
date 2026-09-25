#ifndef MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H
#define MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H

#include <QWidget>
#include <memory>
#include "../service/strategy/CStrategyService.h"

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
		std::unique_ptr<Ui::CViewStrategySettingsClass> m_ui;
		void RefreshFilter();
		void RefreshDetails();
		void RefreshStrategies(const CStrategyService::_TyStrategyList& strategies, const std::string& strError);
		void EditStrategy(bool bNew);
		void RefreshConnection();
		CStrategyService::_TyStrategyList m_strategies;
		_TyCallbackId m_nQueryToken{ 0 };
		QTableWidget* m_pInstances{ nullptr };
		QLabel* m_pDetails{ nullptr };
		QLineEdit* m_pSearch{ nullptr };
		QComboBox* m_pStatus{ nullptr };
};
#endif
