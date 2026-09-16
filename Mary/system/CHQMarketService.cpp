#include "CHQMarketService.h"

#include "CSession.h"
#include "../request/v1/market.pb.h"

#include <cmath>
#include <charconv>
#include <chrono>
#include <deque>
#include <utility>

namespace
{
	std::string ChannelName(MarketBarPeriod period)
	{
		return MarketBarPeriod::Minute == period ? "bar_1m" : "bar_1d";
	}

	std::string HistoryKey(const std::string& strSecurity, MarketBarPeriod period)
	{
		return strSecurity + ":" + ChannelName(period);
	}

	double ScaledPrice(std::int64_t nPrice, std::int32_t nScale)
	{
		double fScale = std::pow(10.0, nScale);
		return 0.0 < fScale ? static_cast<double>(nPrice) / fScale : 0.0;
	}

	std::string ExchangeSuffix(hqmarket::market::v1::Exchange exchange)
	{
		switch (exchange)
		{
		case hqmarket::market::v1::SSE: return "SSE";
		case hqmarket::market::v1::SZSE: return "SZSE";
		case hqmarket::market::v1::BSE: return "BSE";
		case hqmarket::market::v1::HKEX: return "HKEX";
		default: return { };
		}
	}

	Exchange ToExchange(hqmarket::market::v1::Exchange exchange)
	{
		return static_cast<Exchange>(static_cast<int>(exchange));
	}

	std::string SecurityName(const hqmarket::market::v1::Instrument& instrument)
	{
		std::string strExchange = ExchangeSuffix(instrument.exchange());
		return strExchange.empty() ? instrument.symbol() : instrument.symbol() + "." + strExchange;
	}

	std::string MarketCategory(const std::string& strSecurity)
	{
		std::size_t dot = strSecurity.rfind('.');
		if (std::string::npos == dot)
		{
			return "未知";
		}
		std::string strCode = strSecurity.substr(0, dot);
		std::string strExchange = strSecurity.substr(dot + 1);
		if ("BSE" == strExchange)
		{
			return "北交所";
		}
		if ("SSE" == strExchange)
		{
			return (strCode.starts_with("688") || strCode.starts_with("689")) ? "科创板" : "沪A";
		}
		if ("SZSE" == strExchange)
		{
			return (strCode.starts_with("300") || strCode.starts_with("301")) ? "创业板" : "深A";
		}
		return "未知";
	}

	std::uint64_t ParseSequence(const request::RequestParameters& values)
	{
		const auto sequenceIter = values.find("sequence");
		if (values.end() == sequenceIter)
		{
			return 0;
		}
		std::uint64_t sequence = 0;
		const char* pBegin = sequenceIter->second.data();
		const char* pEnd = pBegin + sequenceIter->second.size();
		std::from_chars_result result = std::from_chars(pBegin, pEnd, sequence);
		return (std::errc() == result.ec) && (pEnd == result.ptr) ? sequence : 0;
	}
}

CHQMarketService::CHQMarketService()
{
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Security), "代码", DataType::String });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Name), "名称", DataType::String });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::LastPrice), "最新价", DataType::Double });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Change), "涨跌额", DataType::Double });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Percent), "涨跌幅", DataType::Double });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::PreClose), "昨收", DataType::Double });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Volume), "成交量", DataType::Int64 });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Status), "状态", DataType::String });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Sequence), "序列", DataType::UInt64 });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::ListingStatus), "证券状态", DataType::String });
	m_quoteTable.AddColumn(CDataColumnSchema{ static_cast<_TyDataColumnId>(MarketQuoteColumn::Market), "市场", DataType::String });
}

CHQMarketService::~CHQMarketService()
{
	m_bStopping.store(true);
	m_cv_pendingQuotes.notify_all();
	if (m_quoteWorker.joinable())
	{
		m_quoteWorker.join();
	}
}

void CHQMarketService::Initialize()
{
	if (m_bInitialized)
	{
		return;
	}
	m_bInitialized = true;
	m_bStopping.store(false);
	m_quoteWorker = std::thread(&CHQMarketService::QuoteWorkerLoop, this);
	CSession::InstanceRef().RegisterResponseHandler([this](const SessionResponse& response)
	{
		OnResponse(response);
	});
	CSession::InstanceRef().RegisterStateHandler([this](SessionState state, const std::string&)
	{
		if (SessionState::Ready == state)
		{
			m_nLastQuoteSequence.store(0);
			RestoreSubscriptions();
			QueryInstruments();
		}
	});
	if (CSession::InstanceRef().IsAuthenticated())
	{
		QueryInstruments();
	}
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddQuoteHandler(_TyQuoteHandler&& handler)
{
	return m_quoteHandlers.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveQuoteHandler(_TyHandlerToken token)
{
	m_quoteHandlers.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddDepthHandler(_TyDepthHandler&& handler)
{
	return m_depthHandlers.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveDepthHandler(_TyHandlerToken token)
{
	m_depthHandlers.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddHistoryHandler(_TyHistoryHandler&& handler)
{
	return m_historyHandlers.Subscribe([handler = std::move(handler)](const CMarketHistoryEvent& event)
	{
		handler(event.m_strSecurity, event.m_period, event.m_bars, event.m_strError);
	});
}

void CHQMarketService::RemoveHistoryHandler(_TyHandlerToken token)
{
	m_historyHandlers.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddQuoteTableHandler(_TyQuoteTableHandler&& handler)
{
	return m_quoteTableHandlers.Subscribe([handler = std::move(handler)](const CQuoteTableEvent& event)
	{
		handler(event.m_snapshot, event.m_changes);
	});
}

void CHQMarketService::RemoveQuoteTableHandler(_TyHandlerToken token)
{
	m_quoteTableHandlers.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddInstrumentListHandler(_TyInstrumentListHandler&& handler)
{
	return m_instrumentListHandlers.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveInstrumentListHandler(_TyHandlerToken token)
{
	m_instrumentListHandlers.Unsubscribe(token);
}

void CHQMarketService::RegisterInstrument(const CSecurity& security)
{
	std::string strSecurity = security.String();
	if (strSecurity.empty())
	{
		return;
	}
	bool bRegistered = false;
	{
		std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
		m_instrumentNames.insert_or_assign(strSecurity, security.m_strName);
		m_instrumentStatuses.insert_or_assign(strSecurity, security.m_status);
		m_pendingInstrumentUpdates.emplace(strSecurity);
		bRegistered = m_registeredInstruments.emplace(strSecurity).second;
		if (bRegistered)
		{
			CQuote quote;
			quote.m_strSecurity = strSecurity;
			quote.m_bStale = true;
			m_pendingQuotes.emplace(strSecurity, std::move(quote));
		}
		else if (m_pendingQuotes.end() == m_pendingQuotes.find(strSecurity))
		{
			CQuote quote;
			bool bFoundQuote = false;
			{
				std::lock_guard<std::mutex> quoteLock(m_mtx_quotes);
				const auto quoteIter = m_quotes.find(strSecurity);
				if (m_quotes.end() != quoteIter)
				{
					quote = quoteIter->second;
					bFoundQuote = true;
				}
			}
			quote.m_strSecurity = strSecurity;
			if (!bFoundQuote)
			{
				quote.m_bStale = true;
			}
			m_pendingQuotes.emplace(strSecurity, std::move(quote));
		}
	}
	m_cv_pendingQuotes.notify_one();
}

bool CHQMarketService::SubscribeQuote(const std::string& strSecurity)
{
	request::RequestParameters parameters
	{
		{ "security", strSecurity },
		{ "channel", "quote" }
	};
	return Subscribe("watchlist:" + strSecurity, parameters);
}

bool CHQMarketService::UnsubscribeQuote(const std::string& strSecurity)
{
	request::RequestParameters parameters
	{
		{ "security", strSecurity },
		{ "channel", "quote" }
	};
	return Unsubscribe("watchlist:" + strSecurity, parameters);
}

bool CHQMarketService::SubscribeDepth(const std::string& strSecurity)
{
	request::RequestParameters parameters
	{
		{ "security", strSecurity },
		{ "channel", "depth" }
	};
	return Subscribe("depth:" + strSecurity, parameters);
}

bool CHQMarketService::UnsubscribeDepth(const std::string& strSecurity)
{
	request::RequestParameters parameters
	{
		{ "security", strSecurity },
		{ "channel", "depth" }
	};
	return Unsubscribe("depth:" + strSecurity, parameters);
}

bool CHQMarketService::QueryHistory(const std::string& strSecurity, MarketBarPeriod period, std::int64_t nBeginTime, std::int64_t nEndTime)
{
	return CSession::InstanceRef().SendRequest(request::QueryMarketBars(strSecurity, ChannelName(period), nBeginTime, nEndTime));
}

bool CHQMarketService::QueryInstruments()
{
	return CSession::InstanceRef().SendRequest(request::QueryMarketInstruments());
}

bool CHQMarketService::Subscribe(const std::string& strKey, const request::RequestParameters& param)
{
	if (strKey.empty())
	{
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		m_subscriptions.insert_or_assign(strKey, param);
	}
	return CSession::InstanceRef().SendRequest(request::Subscription(param));
}

bool CHQMarketService::Unsubscribe(const std::string& strKey, const request::RequestParameters& param)
{
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		if (0 == m_subscriptions.erase(strKey))
		{
			return false;
		}
	}
	return CSession::InstanceRef().SendRequest(request::UnSubscription(param));
}

void CHQMarketService::RestoreSubscriptions()
{
	std::unordered_map<std::string, request::RequestParameters> subscriptions;
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		subscriptions = m_subscriptions;
	}
	for (const auto& item : subscriptions)
	{
		CSession::InstanceRef().SendRequest(request::Subscription(item.second));
	}
}

bool CHQMarketService::FindQuote(const std::string& strSecurity, CQuote& result) const
{
	std::lock_guard<std::mutex> lock(m_mtx_quotes);
	const auto mIter = m_quotes.find(strSecurity);
	if (m_quotes.end() == mIter)
	{
		return false;
	}
	result = mIter->second;
	return true;
}

bool CHQMarketService::FindDepth(const std::string& strSecurity, CMarketDepth& result) const
{
	std::lock_guard<std::mutex> lock(m_mtx_depths);
	const auto mIter = m_depths.find(strSecurity);
	if (m_depths.end() == mIter)
	{
		return false;
	}
	result = mIter->second;
	return true;
}

bool CHQMarketService::FindHistory(const std::string& strSecurity, MarketBarPeriod period, std::vector<CMarketBar>& result) const
{
	std::lock_guard<std::mutex> lock(m_mtx_history);
	const auto mIter = m_history.find(HistoryKey(strSecurity, period));
	if (m_history.end() == mIter)
	{
		return false;
	}
	result = mIter->second;
	return true;
}

std::vector<CIndicatorPoint> CHQMarketService::CalculateMovingAverage(const std::vector<CMarketBar>& bars, std::size_t nPeriod) const
{
	std::vector<CIndicatorPoint> values;
	values.reserve(bars.size());
	if (0 == nPeriod)
	{
		return values;
	}

	double fSum = 0.0;
	std::deque<double> window;
	for (const CMarketBar& bar : bars)
	{
		window.emplace_back(bar.m_fClose);
		fSum += bar.m_fClose;
		if (nPeriod < window.size())
		{
			fSum -= window.front();
			window.pop_front();
		}
		bool bValid = nPeriod == window.size();
		values.emplace_back(CIndicatorPoint{ bar.m_nBeginTime, bValid ? fSum / static_cast<double>(nPeriod) : 0.0, bValid });
	}
	return values;
}

CDataSnapshot CHQMarketService::GetQuoteTableSnapshot() const
{
	return m_quoteTable.GetSnapshot();
}

std::vector<CSecurity> CHQMarketService::GetInstruments() const
{
	std::lock_guard<std::mutex> lock(m_mtx_instruments);
	return m_security;
}

CMarketRuntimeMetrics CHQMarketService::GetRuntimeMetrics() const
{
	CMarketRuntimeMetrics metrics;
	metrics.m_nQuoteReceived = m_nQuoteReceived.load();
	metrics.m_nQuoteAccepted = m_nQuoteAccepted.load();
	metrics.m_nQuoteDuplicates = m_nQuoteDuplicates.load();
	metrics.m_nQuoteOutOfOrder = m_nQuoteOutOfOrder.load();
	metrics.m_nQuoteSequenceGaps = m_nQuoteSequenceGaps.load();
	metrics.m_nQuoteWithoutSequence = m_nQuoteWithoutSequence.load();
	metrics.m_nQuoteQueueOverwrites = m_nQuoteQueueOverwrites.load();
	metrics.m_nQuoteQueueDrops = m_nQuoteQueueDrops.load();
	metrics.m_nPendingQuoteLimit = m_nPendingQuoteLimit.load();
	{
		std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
		metrics.m_nPendingQuoteCount = m_pendingQuotes.size();
	}
	return metrics;
}

void CHQMarketService::SetPendingQuoteLimit(std::size_t count)
{
	m_nPendingQuoteLimit.store((std::max)(std::size_t(1), count));
}

bool CHQMarketService::AcceptQuoteSequence(std::uint64_t sequence)
{
	if (0 == sequence)
	{
		++m_nQuoteWithoutSequence;
		return true;
	}
	std::uint64_t previous = m_nLastQuoteSequence.load();
	while (true)
	{
		if (sequence == previous)
		{
			++m_nQuoteDuplicates;
			return false;
		}
		if (sequence < previous)
		{
			++m_nQuoteOutOfOrder;
			return false;
		}
		if (m_nLastQuoteSequence.compare_exchange_weak(previous, sequence))
		{
			break;
		}
	}
	if ((0 != previous) && (previous + 1 < sequence))
	{
		m_nQuoteSequenceGaps.fetch_add(sequence - previous - 1);
	}
	return true;
}

void CHQMarketService::EnqueueQuote(const CQuote& quote)
{
	{
		std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
		const auto quoteIter = m_pendingQuotes.find(quote.m_strSecurity);
		if (m_pendingQuotes.end() != quoteIter)
		{
			quoteIter->second = quote;
			++m_nQuoteQueueOverwrites;
		}
		else if (m_nPendingQuoteLimit.load() <= m_pendingQuotes.size())
		{
			++m_nQuoteQueueDrops;
			return;
		}
		else
		{
			m_pendingQuotes.emplace(quote.m_strSecurity, quote);
		}
	}
	m_cv_pendingQuotes.notify_one();
}

void CHQMarketService::QuoteWorkerLoop()
{
	while (!m_bStopping.load())
	{
		std::unordered_map<std::string, CQuote> quotes;
		{
			std::unique_lock<std::mutex> lock(m_mtx_pendingQuotes);
			m_cv_pendingQuotes.wait_for(lock, std::chrono::milliseconds(20), [this]()
			{
				return m_bStopping.load();
			});
			quotes.swap(m_pendingQuotes);
		}
		if (!quotes.empty())
		{
			FlushQuotes(quotes);
		}
	}
}

void CHQMarketService::FlushQuotes(std::unordered_map<std::string, CQuote>& quotes)
{
	CDataWriteBatch batch = m_quoteTable.BeginWrite();
	for (const auto& item : quotes)
	{
		const CQuote& quote = item.second;
		auto rowIter = m_quoteRowIds.find(quote.m_strSecurity);
		if (m_quoteRowIds.end() == rowIter)
		{
			_TyDataRowId rowId = m_nextQuoteRowId++;
			m_quoteRowIds.emplace(quote.m_strSecurity, rowId);
			std::string strName;
			std::string strListingStatus;
			{
				std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
				m_pendingInstrumentUpdates.erase(quote.m_strSecurity);
				const auto nameIter = m_instrumentNames.find(quote.m_strSecurity);
				if (m_instrumentNames.end() != nameIter)
				{
					strName = nameIter->second;
				}
				const auto statusIter = m_instrumentStatuses.find(quote.m_strSecurity);
				if (m_instrumentStatuses.end() != statusIter)
				{
					strListingStatus = GetMarketStateString(statusIter->second);
				}
			}
			double fChange = quote.m_fLastPrice - quote.m_fPreClose;
			double fPercent = 0.0 == quote.m_fPreClose ? 0.0 : fChange * 100.0 / quote.m_fPreClose;
			batch.AddRow(rowId, { quote.m_strSecurity, strName, quote.m_fLastPrice, fChange, fPercent, quote.m_fPreClose, quote.m_nVolume, std::string(quote.m_bStale ? "已延迟" : "交易中"), quote.m_nSequence, strListingStatus, MarketCategory(quote.m_strSecurity) });
			continue;
		}
		_TyDataRowId rowId = rowIter->second;
		std::string strName;
		std::string strListingStatus;
		bool bMetadataChanged = false;
		{
			std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
			bMetadataChanged = 0 != m_pendingInstrumentUpdates.erase(quote.m_strSecurity);
			if (bMetadataChanged)
			{
				const auto nameIter = m_instrumentNames.find(quote.m_strSecurity);
				if (m_instrumentNames.end() != nameIter)
				{
					strName = nameIter->second;
				}
				const auto statusIter = m_instrumentStatuses.find(quote.m_strSecurity);
				if (m_instrumentStatuses.end() != statusIter)
				{
					strListingStatus = GetMarketStateString(statusIter->second);
				}
			}
		}
		double fChange = quote.m_fLastPrice - quote.m_fPreClose;
		double fPercent = 0.0 == quote.m_fPreClose ? 0.0 : fChange * 100.0 / quote.m_fPreClose;
		if (bMetadataChanged)
		{
			batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Name), strName);
			batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::ListingStatus), strListingStatus);
			batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Market), MarketCategory(quote.m_strSecurity));
		}
		batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::LastPrice), quote.m_fLastPrice);
		batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Change), fChange);
		batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Percent), fPercent);
		batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::PreClose), quote.m_fPreClose);
		batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Volume), quote.m_nVolume);
		batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Status), std::string(quote.m_bStale ? "已延迟" : "交易中"));
		batch.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Sequence), quote.m_nSequence);
	}
	CDataSnapshot snapshot = batch.Commit();
	CDataChangeSet changes = m_quoteTable.GetLastChanges();
	m_quoteTableHandlers.Notify(CQuoteTableEvent{ snapshot, changes });
}

void CHQMarketService::OnResponse(const SessionResponse& response)
{
	if (("instrument_list_response" == response.m_cmd) && response.m_message.has_instrument_list_response())
	{
		const hqmarket::market::v1::InstrumentListResponse& result = response.m_message.instrument_list_response();
		CInstrumentListEvent event;
		event.m_version = result.version();
		event.m_security.reserve(result.instruments_size());
		for (const hqmarket::market::v1::InstrumentInfo& value : result.instruments())
		{
			std::string strExchange = ExchangeSuffix(value.instrument().exchange());
			if (strExchange.empty())
			{
				continue;
			}
			CSecurity instrument(value.instrument().symbol(), ToExchange(value.instrument().exchange()), ParseMarketState(value.status()));
			instrument.m_strName = value.name();
			event.m_security.emplace_back(std::move(instrument));
		}
		{
			std::lock_guard<std::mutex> lock(m_mtx_instruments);
			m_security = event.m_security;
		}
		for (const CSecurity& instrument : event.m_security)
		{
			RegisterInstrument(instrument);
		}
		m_instrumentListHandlers.Notify(event);
		return;
	}

	if (("depth" == response.m_cmd) && response.m_message.has_depth())
	{
		const hqmarket::market::v1::DepthData& depth = response.m_message.depth();
		CMarketDepth value;
		value.m_strSecurity = SecurityName(depth.instrument());
		value.m_nExchangeTime = depth.exchange_time_ms();
		value.m_bStale = depth.stale();
		value.m_bids.reserve(depth.bids_size());
		value.m_asks.reserve(depth.asks_size());
		for (const hqmarket::market::v1::PriceLevel& level : depth.bids())
		{
			value.m_bids.emplace_back(CPriceLevel{ ScaledPrice(level.price(), level.price_scale()), level.volume() });
		}
		for (const hqmarket::market::v1::PriceLevel& level : depth.asks())
		{
			value.m_asks.emplace_back(CPriceLevel{ ScaledPrice(level.price(), level.price_scale()), level.volume() });
		}
		{
			std::lock_guard<std::mutex> lock(m_mtx_depths);
			m_depths.insert_or_assign(value.m_strSecurity, value);
		}
		m_depthHandlers.Notify(value);
		return;
	}

	if (("query_response" == response.m_cmd) && response.m_message.has_query_response())
	{
		const hqmarket::market::v1::QueryResponse& query = response.m_message.query_response();
		MarketBarPeriod period = hqmarket::market::v1::CHANNEL_BAR_1M == query.channel() ? MarketBarPeriod::Minute : MarketBarPeriod::Day;
		std::string strSecurity = SecurityName(query.instrument());
		std::vector<CMarketBar> bars;
		bars.reserve(query.bars_size());
		for (const hqmarket::market::v1::BarData& bar : query.bars())
		{
			bars.emplace_back(CMarketBar{ bar.begin_time_ms(), ScaledPrice(bar.open_price(), bar.price_scale()), ScaledPrice(bar.high_price(), bar.price_scale()), ScaledPrice(bar.low_price(), bar.price_scale()), ScaledPrice(bar.close_price(), bar.price_scale()), bar.volume(), bar.turnover() });
		}
		{
			std::lock_guard<std::mutex> lock(m_mtx_history);
			m_history.insert_or_assign(HistoryKey(strSecurity, period), bars);
		}
		m_historyHandlers.Notify(CMarketHistoryEvent{ strSecurity, period, bars, response.m_error });
		return;
	}

	if (("quote" != response.m_cmd) || !response.m_message.has_quote())
	{
		return;
	}

	const hqmarket::market::v1::QuoteData& quote = response.m_message.quote();
	++m_nQuoteReceived;
	std::uint64_t sequence = ParseSequence(response.m_result);
	if (!AcceptQuoteSequence(sequence))
	{
		return;
	}
	double fScale = std::pow(10.0, quote.price_scale());
	if (0.0 >= fScale)
	{
		return;
	}

	CQuote value;
	value.m_strSecurity = SecurityName(quote.instrument());
	value.m_nSequence = sequence;
	value.m_fLastPrice = static_cast<double>(quote.last_price()) / fScale;
	value.m_fPreClose = static_cast<double>(quote.pre_close()) / fScale;
	value.m_nVolume = quote.volume();
	value.m_bStale = quote.stale();
	++m_nQuoteAccepted;
	EnqueueQuote(value);
	{
		std::lock_guard<std::mutex> lock(m_mtx_quotes);
		m_quotes.insert_or_assign(value.m_strSecurity, value);
	}

	m_quoteHandlers.Notify(value);
}
