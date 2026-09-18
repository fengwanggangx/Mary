#ifndef MARY_COMPONENTS_CUITABLE_H
#define MARY_COMPONENTS_CUITABLE_H

#include "../service/hqmarket/defines_hqmarket.h"
#include <QString>
#include <QWidget>
#include <unordered_set>

class CUICurve;
class CDataTableModel;
class CMarketFilterProxyModel;
class QTableView;
class QModelIndex;
class QLabel;
class QTabBar;
class QLineEdit;
class QSplitter;
enum class CurveMode;

enum class MarketTableMode
{
	Watchlist,
	AShare,
	Constituents
};

class CUITable final : public QWidget
{
	Q_OBJECT
	public:
	explicit CUITable(QWidget* pParent = nullptr);
	CUITable(MarketTableMode mode, QWidget* pParent);
	~CUITable() override;
	void SetSector(const QString& sector);

	private:
	void InitializeUI();
	void BindService();
	void LoadDemoData();
	void UpdateCount();
	void EnsureSelection();
	void RefreshSelection();
	void ApplyTheme();
	void SetMarket(int index);
	void RequestHistory(CurveMode mode);
	void HandleHistory(std::uint64_t requestId, const std::string& security, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& error);
	void HandleQuoteTable(const CDataTableView& view, const CDataChangeSet& changes);
	CSecurity GetSecurity(const QModelIndex& index) const;
	void changeEvent(QEvent* event) override;
	void showEvent(QShowEvent* event) override;

	MarketTableMode m_mode{ MarketTableMode::Watchlist };
	bool m_demo{ false };
	bool m_layoutInitialized{ false };
	QSplitter* m_splitter{ nullptr };
	QString m_sector;
	std::string m_selectedSecurity;
	CDataTable m_demoTable;
	std::unordered_set<std::string> m_watchlist;
	CUICurve* m_intraday{ nullptr };
	CUICurve* m_candles{ nullptr };
	QTableView* m_table{ nullptr };
	CDataTableModel* m_model{ nullptr };
	CMarketFilterProxyModel* m_proxy{ nullptr };
	QLabel* m_title{ nullptr };
	QLabel* m_count{ nullptr };
	QLabel* m_stockTitle{ nullptr };
	QLabel* m_price{ nullptr };
	QLabel* m_chartState{ nullptr };
	QTabBar* m_marketTabs{ nullptr };
	QTabBar* m_periodTabs{ nullptr };
	std::uint64_t m_quoteTableToken{ 0 };
	std::uint64_t m_historyToken{ 0 };
	std::uint64_t m_minuteRequestId{ 0 };
	std::uint64_t m_dayRequestId{ 0 };
};

#endif
