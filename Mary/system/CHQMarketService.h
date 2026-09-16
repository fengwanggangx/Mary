#ifndef MARY_SYSTEM_CHQMARKETSERVICE_H
#define MARY_SYSTEM_CHQMARKETSERVICE_H

#include "../common/ISingleton.h"
#include "../basic/CallbackRegistry.h"
#include "../basic/CDatable.h"
#include "../request/RequestCenter.h"
#include "defines_hqmarket.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>

class CRequest;

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

	void RegisterInstrument(const CSecurity& info);
	bool SubscribeQuote(const CSecurity& info);
	bool UnsubscribeQuote(const CSecurity& info);
	bool SubscribeDepth(const CSecurity& info);
	bool UnsubscribeDepth(const CSecurity& info);

	bool QueryHistory(const CSecurity& info, MarketBarPeriod period, std::int64_t nBeginTime, std::int64_t nEndTime);
	bool QueryInstruments();

	bool FindQuote(const CSecurity& info, CQuote& result) const;
	bool FindDepth(const CSecurity& info, CMarketDepth& result) const;
	bool FindHistory(const CSecurity& info, MarketBarPeriod period, std::vector<CMarketBar>& result) const;

	std::vector<CIndicatorPoint> CalculateMovingAverage(const std::vector<CMarketBar>& bars, std::size_t nPeriod) const;

	CDataSnapshot GetQuoteTableSnapshot() const;
	std::vector<CSecurity> GetInstruments() const;
	CMarketRuntimeMetrics GetRuntimeMetrics() const;

	void SetPendingQuoteLimit(std::size_t count);

private:
	void OnResponse(const CRequest& req);

	bool Subscribe(const std::string& strKey, const request::_TyParams& param);
	bool Unsubscribe(const std::string& strKey, const request::_TyParams& param);
	void RestoreSubscriptions();

	bool AcceptQuoteSequence(std::uint64_t sequence);

	void EnqueueQuote(const CQuote& quote);
	void QuoteWorkerLoop();
	void FlushQuotes(std::unordered_map<std::string, CQuote>& quotes);

private:
	bool m_bInitialized{ false }; // 服务是否已初始化

	mutable std::shared_mutex m_smtx_quotes;
	std::unordered_map<std::string, CQuote> m_quotes; // 各证券最新行情

	mutable std::shared_mutex m_smtx_depths;
	std::unordered_map<std::string, CMarketDepth> m_depths; // 各证券最新盘口

	mutable std::shared_mutex m_smtx_history;
	std::unordered_map<std::string, std::vector<CMarketBar>> m_history; // 各证券历史 K 线

	std::mutex m_mtx_subscriptions;
	std::unordered_map<std::string, request::_TyParams> m_subscriptions; // 当前有效订阅

	CallbackRegistry<CQuote> m_quoteHandlers; // 最新行情回调注册表
	CallbackRegistry<CMarketDepth> m_depthHandlers; // 盘口深度回调注册表
	CallbackRegistry<CMarketHistoryEvent> m_historyHandlers; // 历史行情回调注册表
	CallbackRegistry<CQuoteTableEvent> m_quoteTableHandlers; // 行情表变更回调注册表
	CallbackRegistry<CInstrumentListEvent> m_instrumentListHandlers; // 证券列表回调注册表

	mutable std::shared_mutex m_smtx_instruments;
	std::vector<CSecurity> m_security; // 当前证券列表

	CDataTable m_quoteTable; // 行情展示数据表

	mutable std::mutex m_mtx_pendingQuotes; // 待处理行情及证券元数据锁
	std::condition_variable m_cv_pendingQuotes; // 行情处理线程唤醒条件
	std::unordered_map<std::string, CQuote> m_pendingQuotes; // 等待批量刷新的行情
	std::unordered_map<std::string, _TyDataRowId> m_quoteRowIds; // 证券对应的数据表行号
	std::unordered_map<std::string, std::string> m_instrumentNames; // 证券名称缓存
	std::unordered_map<std::string, MarketState> m_instrumentStatuses; // 证券状态缓存
	std::unordered_set<std::string> m_pendingInstrumentUpdates; // 等待刷新的证券元数据
	std::unordered_set<std::string> m_registeredInstruments; // 已注册证券集合

	std::thread m_quoteWorker; // 行情批量处理线程
	std::atomic_bool m_bStopping{ false }; // 行情线程停止标志

	CAtomMarketRuntimeMetrics m_runtimeMetrics; // 线程安全的行情运行统计

	_TyDataRowId m_nextQuoteRowId{ 1 }; // 下一个行情表行号
};

#endif
