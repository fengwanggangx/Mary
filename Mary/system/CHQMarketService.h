#ifndef MARY_SYSTEM_CHQMARKETSERVICE_H
#define MARY_SYSTEM_CHQMARKETSERVICE_H

#include "../common/ISingleton.h"
#include "../basic/CallbackRegistry.h"
#include "../basic/CDatable.h"
#include "../request/RequestCenter.h"
#include "../request/MarketTypes.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>

struct SessionResponse;

struct CQuote
{
	std::string m_strSecurity;
	std::uint64_t m_nSequence{ 0 };
	double m_fLastPrice{ 0.0 };
	double m_fPreClose{ 0.0 };
	std::int64_t m_nVolume{ 0 };
	bool m_bStale{ false };
};

enum class MarketQuoteColumn : _TyDataColumnId
{
	Security = 1,
	Name,
	LastPrice,
	Change,
	Percent,
	PreClose,
	Volume,
	Status,
	Sequence,
	ListingStatus,
	Market
};

enum class MarketBarPeriod
{
	Minute,
	Day
};

struct CPriceLevel
{
	double m_fPrice{ 0.0 };
	std::int64_t m_nVolume{ 0 };
};

struct CMarketDepth
{
	std::string m_strSecurity;
	std::int64_t m_nExchangeTime{ 0 };
	std::vector<CPriceLevel> m_bids;
	std::vector<CPriceLevel> m_asks;
	bool m_bStale{ false };
};

struct CMarketBar
{
	std::int64_t m_nBeginTime{ 0 };
	double m_fOpen{ 0.0 };
	double m_fHigh{ 0.0 };
	double m_fLow{ 0.0 };
	double m_fClose{ 0.0 };
	std::int64_t m_nVolume{ 0 };
	std::int64_t m_nTurnover{ 0 };
};

struct CIndicatorPoint
{
	std::int64_t m_nTime{ 0 };
	double m_fValue{ 0.0 };
	bool m_bValid{ false };
};

struct CMarketHistoryEvent
{
	std::string m_strSecurity;
	MarketBarPeriod m_period{ MarketBarPeriod::Day };
	std::vector<CMarketBar> m_bars;
	std::string m_strError;
};

struct CQuoteTableEvent
{
	CDataSnapshot m_snapshot;
	CDataChangeSet m_changes;
};

struct CInstrumentListEvent
{
	std::int64_t m_version{ 0 };
	std::vector<CSecurity> m_security;
};

struct CMarketRuntimeMetrics
{
	std::uint64_t m_nQuoteReceived{ 0 };
	std::uint64_t m_nQuoteAccepted{ 0 };
	std::uint64_t m_nQuoteDuplicates{ 0 };
	std::uint64_t m_nQuoteOutOfOrder{ 0 };
	std::uint64_t m_nQuoteSequenceGaps{ 0 };
	std::uint64_t m_nQuoteWithoutSequence{ 0 };
	std::uint64_t m_nQuoteQueueOverwrites{ 0 };
	std::uint64_t m_nQuoteQueueDrops{ 0 };
	std::size_t m_nPendingQuoteCount{ 0 };
	std::size_t m_nPendingQuoteLimit{ 0 };
};

class CHQMarketService final : public ISingleton<CHQMarketService>
{
	DECLARE_SINGLE_DFAULT(CHQMarketService)

public:
	using _TyHandlerToken = _TyCallbackId;
	using _TyQuoteHandler = std::function<void(const CQuote&)>;
	using _TyDepthHandler = std::function<void(const CMarketDepth&)>;
	using _TyHistoryHandler = std::function<void(const std::string&, MarketBarPeriod, const std::vector<CMarketBar>&, const std::string&)>;
	using _TyQuoteTableHandler = std::function<void(const CDataSnapshot&, const CDataChangeSet&)>;
	using _TyInstrumentListHandler = std::function<void(const CInstrumentListEvent&)>;

	void Initialize();
	_TyHandlerToken AddQuoteHandler(_TyQuoteHandler&& handler);
	void RemoveQuoteHandler(_TyHandlerToken token);
	_TyHandlerToken AddDepthHandler(_TyDepthHandler&& handler);
	void RemoveDepthHandler(_TyHandlerToken token);
	_TyHandlerToken AddHistoryHandler(_TyHistoryHandler&& handler);
	void RemoveHistoryHandler(_TyHandlerToken token);
	_TyHandlerToken AddQuoteTableHandler(_TyQuoteTableHandler&& handler);
	void RemoveQuoteTableHandler(_TyHandlerToken token);
	_TyHandlerToken AddInstrumentListHandler(_TyInstrumentListHandler&& handler);
	void RemoveInstrumentListHandler(_TyHandlerToken token);
	void RegisterInstrument(const CSecurity& security);
	bool SubscribeQuote(const std::string& strSecurity);
	bool UnsubscribeQuote(const std::string& strSecurity);
	bool SubscribeDepth(const std::string& strSecurity);
	bool UnsubscribeDepth(const std::string& strSecurity);
	bool QueryHistory(const std::string& strSecurity, MarketBarPeriod period, std::int64_t nBeginTime, std::int64_t nEndTime);
	bool QueryInstruments();
	bool FindQuote(const std::string& strSecurity, CQuote& result) const;
	bool FindDepth(const std::string& strSecurity, CMarketDepth& result) const;
	bool FindHistory(const std::string& strSecurity, MarketBarPeriod period, std::vector<CMarketBar>& result) const;
	std::vector<CIndicatorPoint> CalculateMovingAverage(const std::vector<CMarketBar>& bars, std::size_t nPeriod) const;
	CDataSnapshot GetQuoteTableSnapshot() const;
	std::vector<CSecurity> GetInstruments() const;
	CMarketRuntimeMetrics GetRuntimeMetrics() const;
	void SetPendingQuoteLimit(std::size_t count);

private:
	void OnResponse(const SessionResponse& response);
	bool Subscribe(const std::string& strKey, const request::RequestParameters& param);
	bool Unsubscribe(const std::string& strKey, const request::RequestParameters& param);
	void RestoreSubscriptions();
	bool AcceptQuoteSequence(std::uint64_t sequence);
	void EnqueueQuote(const CQuote& quote);
	void QuoteWorkerLoop();
	void FlushQuotes(std::unordered_map<std::string, CQuote>& quotes);

private:
	bool m_bInitialized{ false };
	mutable std::mutex m_mtx_quotes;
	std::unordered_map<std::string, CQuote> m_quotes;
	mutable std::mutex m_mtx_depths;
	std::unordered_map<std::string, CMarketDepth> m_depths;
	mutable std::mutex m_mtx_history;
	std::unordered_map<std::string, std::vector<CMarketBar>> m_history;
	std::mutex m_mtx_subscriptions;
	std::unordered_map<std::string, request::RequestParameters> m_subscriptions;
	CallbackRegistry<CQuote> m_quoteHandlers;
	CallbackRegistry<CMarketDepth> m_depthHandlers;
	CallbackRegistry<CMarketHistoryEvent> m_historyHandlers;
	CallbackRegistry<CQuoteTableEvent> m_quoteTableHandlers;
	CallbackRegistry<CInstrumentListEvent> m_instrumentListHandlers;
	mutable std::mutex m_mtx_instruments;
	std::vector<CSecurity> m_security;
	CDataTable m_quoteTable;
	mutable std::mutex m_mtx_pendingQuotes;
	std::condition_variable m_cv_pendingQuotes;
	std::unordered_map<std::string, CQuote> m_pendingQuotes;
	std::unordered_map<std::string, _TyDataRowId> m_quoteRowIds;
	std::unordered_map<std::string, std::string> m_instrumentNames;
	std::unordered_map<std::string, MarketState> m_instrumentStatuses;
	std::unordered_set<std::string> m_pendingInstrumentUpdates;
	std::unordered_set<std::string> m_registeredInstruments;
	std::thread m_quoteWorker;
	std::atomic_bool m_bStopping{ false };
	std::atomic_uint64_t m_nLastQuoteSequence{ 0 };
	std::atomic_uint64_t m_nQuoteReceived{ 0 };
	std::atomic_uint64_t m_nQuoteAccepted{ 0 };
	std::atomic_uint64_t m_nQuoteDuplicates{ 0 };
	std::atomic_uint64_t m_nQuoteOutOfOrder{ 0 };
	std::atomic_uint64_t m_nQuoteSequenceGaps{ 0 };
	std::atomic_uint64_t m_nQuoteWithoutSequence{ 0 };
	std::atomic_uint64_t m_nQuoteQueueOverwrites{ 0 };
	std::atomic_uint64_t m_nQuoteQueueDrops{ 0 };
	std::atomic_size_t m_nPendingQuoteLimit{ 10000 };
	_TyDataRowId m_nextQuoteRowId{ 1 };
};

#endif
