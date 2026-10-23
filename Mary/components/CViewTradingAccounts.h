#pragma once
#include "../service/account/CTradeAccountService.h"
#include <QWidget>
#include <array>
#include <functional>
class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QTableView;
class QTableWidget;
class QTabWidget;
class QStandardItemModel;
class CTradingAccountModel;
class CViewTradingAccounts final : public QWidget
{
	friend class CTradingAccountUiTests;

	public:
	explicit CViewTradingAccounts(QWidget* parent = nullptr);
	~CViewTradingAccounts() override;
	void SetStrategyNavigation(std::function<void(std::uint64_t)>&& handler);

	protected:
	void showEvent(QShowEvent* event) override;

	private:
	void Refresh();
	void RefreshRows();
	void RefreshSelection();
	void ClearDetails();
	void RefreshButtons();
	void EditAccount(bool bNew);
	void Operate(_TyRequest type);
	void HandleResult(const CTradeAccountService::CResult& result);
	void RefreshStrategies();
	const request::TradeAccountRecord* SelectedAccount() const;
	CTradingAccountModel* m_model{ nullptr };
	QTableView* m_table{ nullptr };
	QLineEdit* m_search{ nullptr };
	QComboBox* m_type{ nullptr };
	QPushButton* m_add{ nullptr };
	QPushButton* m_edit{ nullptr };
	QPushButton* m_connect{ nullptr };
	QPushButton* m_disconnect{ nullptr };
	QLabel* m_heading{ nullptr };
	QLabel* m_notice{ nullptr };
	QLabel* m_updated{ nullptr };
	QLabel* m_frozen{ nullptr };
	QLabel* m_basic{ nullptr };
	QTableWidget* m_strategies{ nullptr };
	QTableWidget* m_logs{ nullptr };
	QTabWidget* m_details{ nullptr };
	std::array<QLabel*, 4> m_summary{};
	std::array<QLabel*, 4> m_funds{};
	std::vector<request::TradeAccountRecord> m_accounts;
	std::vector<request::TradeAccountRecord> m_visible;
	std::uint64_t m_selectedId{ 0 };
	std::uint64_t m_operationId{ 0 };
	std::uint64_t m_snapshotId{ 0 };
	std::uint64_t m_logsId{ 0 };
	_TyCallbackId m_accountToken{ 0 };
	_TyCallbackId m_strategyToken{ 0 };
	std::function<void(std::uint64_t)> m_navigate;
};
