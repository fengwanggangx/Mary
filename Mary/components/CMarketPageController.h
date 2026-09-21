#ifndef MARY_COMPONENTS_CMARKETPAGECONTROLLER_H
#define MARY_COMPONENTS_CMARKETPAGECONTROLLER_H

#include "../service/hqmarket/CHQMarketService.h"
#include <QObject>
#include <QString>
#include <unordered_set>

class CUITable;
class CUICurve;
class CDataTableModel;
class CMarketFilterProxyModel;
class QLabel;
enum class CurveMode;

enum class MarketTableMode
{
	Watchlist,
	AShare,
	Constituents
};

// Shared page-side market logic, with no layout or search controls.
class CMarketPageController final : public QObject
{
  public:
	CMarketPageController(MarketTableMode mode, CUITable* pTable, QObject* pParent);
	~CMarketPageController() override;
	void SetConstituents(const std::vector<CSecurity>& securities);
	void SetMarket(int nIndex);
	void ToggleWatchlist(const QString& strCode);
	void SetCharts(QLabel* pTitle, QLabel* pPrice, QLabel* pState, CUICurve* pIntraday, CUICurve* pCandles);
	void RequestHistory(CurveMode mode);

  private:
	void BindService();
	void EnsureSelection();
	void RefreshSelection();
	void HandleHistory(std::uint64_t nRequestId, const std::string& strSecurity, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& strError);
	void OnQuoteTableUpdate(const CDataTableView& view, const CDataChangeSet& changes);
	CSecurity GetSecurity(const QModelIndex& index) const;

	MarketTableMode m_mode;
	std::string m_selectedSecurity;
	std::unordered_set<std::string> m_watchlist;
	std::unordered_set<std::string> m_constituents;
	CUITable* m_table{ nullptr };
	CDataTableModel* m_model{ nullptr };
	CMarketFilterProxyModel* m_proxy{ nullptr };
	CUICurve* m_intraday{ nullptr };
	CUICurve* m_candles{ nullptr };
	QLabel* m_stockTitle{ nullptr };
	QLabel* m_price{ nullptr };
	QLabel* m_chartState{ nullptr };
	std::uint64_t m_quoteTableToken{ 0 };
	std::uint64_t m_historyToken{ 0 };
	std::uint64_t m_minuteRequestId{ 0 };
	std::uint64_t m_dayRequestId{ 0 };
};

#endif
