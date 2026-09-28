#ifndef MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H
#define MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H

#include <QWidget>
#include <QString>
#include <cstdint>
#include <memory>
#include <unordered_map>
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
		void RefreshRuntime(const CStrategyService::_TyRuntimeMap& runtimes, const std::string& strError);
		void RefreshRows();
		void HandleOperation(const CStrategyService::COperationResult& result);
		void EditStrategy(bool bNew);
		void DeleteStrategy();
		void ControlStrategy(const std::string& command);
		void RefreshConnection();
		CStrategyService::_TyStrategyList m_strategies;
		CStrategyService::_TyRuntimeMap m_runtimes;
		std::unordered_map<std::uint64_t, QString> m_pendingOperations;
		_TyCallbackId m_nQueryToken{ 0 };
		_TyCallbackId m_nRuntimeToken{ 0 };
		_TyCallbackId m_nOperationToken{ 0 };
		bool m_bConfigStale{ true };
		bool m_bRuntimeStale{ true };
		bool m_bWasConnected{ false };
		int m_refreshTicks{ 0 };
		std::uint64_t m_selectedStrategyId{ 0 };
		QTableWidget* m_pInstances{ nullptr };
		QLabel* m_pDetails{ nullptr };
		QLineEdit* m_pSearch{ nullptr };
		QComboBox* m_pStatus{ nullptr };
};
#endif
