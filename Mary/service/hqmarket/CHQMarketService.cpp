#include "CHQMarketService.h"

#include "../../system/CSession.h"
#include "../../request/v1/market.pb.h"

#include <cmath>
#include <charconv>
#include <chrono>
#include <deque>
#include <utility>
#include <common/container.h>

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

	std::string ExchangeSuffix(_TyMarketExchange exchange)
	{
		switch (exchange)
		{
		case hqmarket::market::v1::SSE:
			return "SSE";
		case hqmarket::market::v1::SZSE:
			return "SZSE";
		case hqmarket::market::v1::BSE:
			return "BSE";
		case hqmarket::market::v1::HKEX:
			return "HKEX";
		default:
			return {};
		}
	}

	Exchange ToExchange(_TyMarketExchange exchange)
	{
		return static_cast<Exchange>(static_cast<int>(exchange));
	}

	std::string SecurityName(const _TySecurity& security)
	{
		std::string strExchange = ExchangeSuffix(security.exchange());
		return strExchange.empty() ? security.symbol() : security.symbol() + "." + strExchange;
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
			if (strCode.starts_with("000"))
			{
				return "指数";
			}
			return (strCode.starts_with("688") || strCode.starts_with("689")) ? "科创板" : "沪A";
		}
		if ("SZSE" == strExchange)
		{
			if (strCode.starts_with("399"))
			{
				return "指数";
			}
			return (strCode.starts_with("300") || strCode.starts_with("301")) ? "创业板" : "深A";
		}
		return "未知";
	}

	std::uint64_t ParseSequence(const request::_TyParams& values)
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

	CSectorInfo ParseSector(const hqmarket::market::v1::SectorInfo& value, SectorType type)
	{
		CSectorInfo sector;
		sector.m_type = type;
		sector.m_strCode = value.code();
		sector.m_strName = value.name();
		sector.m_fChangePercent = ScaledPrice(value.change_percent(), value.percent_scale());
		sector.m_nRisingCount = value.rising_count();
		sector.m_nFallingCount = value.falling_count();
		sector.m_nFlatCount = value.flat_count();
		sector.m_nMemberCount = value.member_count();
		sector.m_leadingSecurity = CSecurity(value.leading_security().symbol(), ToExchange(value.leading_security().exchange()));
		sector.m_strLeadingName = value.leading_name();
		sector.m_nSnapshotTime = value.snapshot_time_ms();
		return sector;
	}
} // namespace

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
	CSession::InstanceRef().RegisterResponseHandler(std::bind_front(&CHQMarketService::OnRequestReply, this));
	CSession::InstanceRef().RegisterStateHandler([this](SessionState state, const std::string&)
												 {
		if (SessionState::Ready == state)
		{
			m_runtimeMetrics.m_nLastQuoteSequence.store(0);
			RestoreSubscriptions();
			QuerySecurities();
		}
		else if ((SessionState::Disconnected == state) || (SessionState::Reconnecting == state) || (SessionState::Stopping == state))
		{
			std::vector<CQuote> quotes;
			{
				std::unique_lock lock(m_mtx_quotes);
				quotes.reserve(m_quotes.size());
				for (auto& [strSecurity, quote] : m_quotes)
				{
					quote.m_bStale = true;
					quotes.emplace_back(quote);
				}
			}
			for (const auto& quote : quotes)
			{
				EnqueueQuote(quote);
			}
		} });
	if (CSession::InstanceRef().IsAuthenticated())
	{
		QuerySecurities();
	}
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddQuoteHandler(_TyQuoteHandler&& handler)
{
	return m_pump_quote.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveQuoteHandler(_TyHandlerToken token)
{
	m_pump_quote.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddDepthHandler(_TyDepthHandler&& handler)
{
	return m_pump_depth.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveDepthHandler(_TyHandlerToken token)
{
	m_pump_depth.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddHistoryHandler(_TyHistoryHandler&& handler)
{
	return m_pump_history.Subscribe([handler = std::move(handler)](const CMarketHistoryEvent& event)
									{ handler(event.m_requestId, event.m_strSecurity, event.m_period, event.m_bars, event.m_strError); });
}

void CHQMarketService::RemoveHistoryHandler(_TyHandlerToken token)
{
	m_pump_history.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddQuoteTableHandler(_TyQuoteTableHandler&& handler)
{
	return m_pump_quote_table.Subscribe([handler = std::move(handler)](const CQuoteTableEvent& event)
										{ handler(event.m_view, event.m_changes); });
}

void CHQMarketService::RemoveQuoteTableHandler(_TyHandlerToken token)
{
	m_pump_quote_table.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddSecurityListHandler(_TySecurityListHandler&& handler)
{
	return m_pump_security_list.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveSecurityListHandler(_TyHandlerToken token)
{
	m_pump_security_list.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddSectorListHandler(_TySectorListHandler&& handler)
{
	return m_pump_sector_list.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveSectorListHandler(_TyHandlerToken token)
{
	m_pump_sector_list.Unsubscribe(token);
}

CHQMarketService::_TyHandlerToken CHQMarketService::AddSectorConstituentsHandler(_TySectorConstituentsHandler&& handler)
{
	return m_pump_sector_constituents.Subscribe(std::move(handler));
}

void CHQMarketService::RemoveSectorConstituentsHandler(_TyHandlerToken token)
{
	m_pump_sector_constituents.Unsubscribe(token);
}

void CHQMarketService::RegisterSecurity(const CSecurity& info)
{
	std::string strKey = info.String();
	if (strKey.empty())
	{
		return;
	}
	bool bRegistered = false;
	{
		std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
		m_securityNames.insert_or_assign(strKey, info.m_strName);
		m_securityStatuses.insert_or_assign(strKey, info.m_status);
		m_pendingSecurityUpdates.emplace(strKey);
		bRegistered = m_registeredSecurities.emplace(strKey).second;
		if (bRegistered)
		{
			CQuote& quote = container::emplace_back(m_pendingQuotes, strKey, info);
			quote.m_bStale = true;
		}
		else if (m_pendingQuotes.end() == m_pendingQuotes.find(strKey))
		{
			CQuote& quote = container::emplace_back(m_pendingQuotes, strKey);
			bool bFind = false;
			{
				std::shared_lock quoteLock(m_mtx_quotes);
				const auto mIter = m_quotes.find(strKey);
				if (m_quotes.end() != mIter)
				{
					quote = mIter->second;
					bFind = true;
				}
			}
			quote.m_security = info;
			if (!bFind)
			{
				quote.m_bStale = true;
			}
		}
	}
	m_cv_pendingQuotes.notify_one();
}

bool CHQMarketService::SubscribeQuote(const CSecurity& info)
{
	std::string strSecurity = info.String();
	request::_TyParams parameters{
		{ "security", strSecurity },
		{ "channel", "quote" }
	};
	return Subscribe("watchlist:" + strSecurity, parameters);
}

bool CHQMarketService::UnsubscribeQuote(const CSecurity& info)
{
	std::string strSecurity = info.String();
	request::_TyParams parameters{
		{ "security", strSecurity },
		{ "channel", "quote" }
	};
	return Unsubscribe("watchlist:" + strSecurity, parameters);
}

bool CHQMarketService::SubscribeDepth(const CSecurity& info)
{
	std::string strSecurity = info.String();
	request::_TyParams parameters{
		{ "security", strSecurity },
		{ "channel", "depth" }
	};
	return Subscribe("depth:" + strSecurity, parameters);
}

bool CHQMarketService::UnsubscribeDepth(const CSecurity& info)
{
	std::string strSecurity = info.String();
	request::_TyParams parameters{
		{ "security", strSecurity },
		{ "channel", "depth" }
	};
	return Unsubscribe("depth:" + strSecurity, parameters);
}

bool CHQMarketService::QueryHistory(const CSecurity& info, MarketBarPeriod period, std::int64_t nBeginTime, std::int64_t nEndTime, _TyRequestId* requestId)
{
	CRequest request = request::QueryMarketBars(info, ChannelName(period), nBeginTime, nEndTime);
	if (nullptr != requestId)
	{
		*requestId = request.GetId();
	}
	{
		std::lock_guard<std::mutex> lock(m_mtx_historyRequests);
		m_historyRequests.emplace(request.GetId(), CMarketHistoryEvent{ request.GetId(), info.String(), period });
	}
	if (CSession::InstanceRef().SendRequest(request))
	{
		return true;
	}
	request.SetReturnData("error_code", "-1");
	request.SetReturnData("error_message", "历史行情请求发送失败");
	OnRequestReply(request);
	return false;
}

bool CHQMarketService::QuerySecurities()
{
	return CSession::InstanceRef().SendRequest(request::QueryMarketSecurities());
}

bool CHQMarketService::QuerySectors(SectorType type)
{
	CRequest req = request::QueryMarketSectors(type);
	m_nLatestSectorListRequest.store(req.GetId());
	if (CSession::InstanceRef().SendRequest(req))
	{
		return true;
	}
	req.SetReturnData("error_code", "-1");
	req.SetReturnData("error_message", "板块查询请求发送失败");
	OnRequestReply(req);
	return false;
}

bool CHQMarketService::QuerySectorConstituents(SectorType type, const std::string& strSectorCode)
{
	CRequest req = request::QueryMarketSectorConstituents(type, strSectorCode);
	m_nLatestSectorConstituentsRequest.store(req.GetId());
	if (CSession::InstanceRef().SendRequest(req))
	{
		return true;
	}
	req.SetReturnData("error_code", "-1");
	req.SetReturnData("error_message", "成分股查询请求发送失败");
	OnRequestReply(req);
	return false;
}

bool CHQMarketService::Subscribe(const std::string& strKey, const request::_TyParams& param)
{
	if (strKey.empty())
	{
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		const auto existing = m_subscriptions.find(strKey);
		if ((m_subscriptions.end() != existing) && (param == existing->second))
		{
			return CSession::InstanceRef().IsAuthenticated();
		}
		m_subscriptions.insert_or_assign(strKey, param);
	}
	return CSession::InstanceRef().SendRequest(request::Subscription(param));
}

bool CHQMarketService::Unsubscribe(const std::string& strKey, const request::_TyParams& param)
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
	std::unordered_map<std::string, request::_TyParams> subscriptions;
	{
		std::lock_guard<std::mutex> lock(m_mtx_subscriptions);
		subscriptions = m_subscriptions;
	}
	for (const auto& item : subscriptions)
	{
		CSession::InstanceRef().SendRequest(request::Subscription(item.second));
	}
}

bool CHQMarketService::FindQuote(const CSecurity& info, CQuote& result) const
{
	std::shared_lock lock(m_mtx_quotes);
	const auto mIter = m_quotes.find(info.String());
	if (m_quotes.end() == mIter)
	{
		return false;
	}
	result = mIter->second;
	return true;
}

bool CHQMarketService::FindDepth(const CSecurity& info, CMarketDepth& result) const
{
	std::shared_lock lock(m_mtx_depths);
	const auto mIter = m_depths.find(info.String());
	if (m_depths.end() == mIter)
	{
		return false;
	}
	result = mIter->second;
	return true;
}

bool CHQMarketService::FindHistory(const CSecurity& info, MarketBarPeriod period, std::vector<CMarketBar>& result) const
{
	std::shared_lock lock(m_mtx_history);
	const auto mIter = m_history.find(HistoryKey(info.String(), period));
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

CDataTableView CHQMarketService::GetQuoteTableView() const
{
	return m_quoteTable.GetView();
}

std::vector<CSecurity> CHQMarketService::GetSecurities() const
{
	std::shared_lock lock(m_mtx_securities);
	return m_securities;
}

std::vector<CSectorInfo> CHQMarketService::GetSectors() const
{
	std::shared_lock lock(m_mtx_sectors);
	return m_sectors;
}

CMarketRuntimeMetrics CHQMarketService::GetRuntimeMetrics() const
{
	CMarketRuntimeMetrics metrics;
	metrics.m_nQuoteReceived = m_runtimeMetrics.m_nQuoteReceived.load();
	metrics.m_nQuoteAccepted = m_runtimeMetrics.m_nQuoteAccepted.load();
	metrics.m_nQuoteDuplicates = m_runtimeMetrics.m_nQuoteDuplicates.load();
	metrics.m_nQuoteOutOfOrder = m_runtimeMetrics.m_nQuoteOutOfOrder.load();
	metrics.m_nQuoteSequenceGaps = m_runtimeMetrics.m_nQuoteSequenceGaps.load();
	metrics.m_nQuoteWithoutSequence = m_runtimeMetrics.m_nQuoteWithoutSequence.load();
	metrics.m_nQuoteQueueOverwrites = m_runtimeMetrics.m_nQuoteQueueOverwrites.load();
	metrics.m_nQuoteQueueDrops = m_runtimeMetrics.m_nQuoteQueueDrops.load();
	metrics.m_nPendingQuoteLimit = m_runtimeMetrics.m_nPendingQuoteLimit.load();
	{
		std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
		metrics.m_nPendingQuoteCount = m_pendingQuotes.size();
	}
	return metrics;
}

void CHQMarketService::SetPendingQuoteLimit(std::size_t count)
{
	m_runtimeMetrics.m_nPendingQuoteLimit.store((std::max)(std::size_t(1), count));
}

bool CHQMarketService::AcceptQuoteSequence(std::uint64_t sequence)
{
	if (0 == sequence)
	{
		++m_runtimeMetrics.m_nQuoteWithoutSequence;
		return true;
	}
	std::uint64_t previous = m_runtimeMetrics.m_nLastQuoteSequence.load();
	while (true)
	{
		if (sequence == previous)
		{
			++m_runtimeMetrics.m_nQuoteDuplicates;
			return false;
		}
		if (sequence < previous)
		{
			++m_runtimeMetrics.m_nQuoteOutOfOrder;
			return false;
		}
		if (m_runtimeMetrics.m_nLastQuoteSequence.compare_exchange_weak(previous, sequence))
		{
			break;
		}
	}
	if ((0 != previous) && (previous + 1 < sequence))
	{
		m_runtimeMetrics.m_nQuoteSequenceGaps.fetch_add(sequence - previous - 1);
	}
	return true;
}

void CHQMarketService::EnqueueQuote(const CQuote& quote)
{
	std::string strSecurity = quote.m_security.String();
	{
		std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
		const auto quoteIter = m_pendingQuotes.find(strSecurity);
		if (m_pendingQuotes.end() != quoteIter)
		{
			quoteIter->second = quote;
			++m_runtimeMetrics.m_nQuoteQueueOverwrites;
		}
		else if (m_runtimeMetrics.m_nPendingQuoteLimit.load() <= m_pendingQuotes.size())
		{
			++m_runtimeMetrics.m_nQuoteQueueDrops;
			return;
		}
		else
		{
			m_pendingQuotes.emplace(std::move(strSecurity), quote);
		}
	}
	m_cv_pendingQuotes.notify_one();
}

void CHQMarketService::QuoteWorkerLoop()
{
	while (!m_bStopping.load())
	{
		decltype(m_pendingQuotes) quotes;
		{
			std::unique_lock<std::mutex> lock(m_mtx_pendingQuotes);
			m_cv_pendingQuotes.wait_for(lock, std::chrono::milliseconds(20), [this]()
										{ return m_bStopping.load(); });
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
	CDataTableWriter writer = m_quoteTable.BeginWrite();
	for (const auto& item : quotes)
	{
		const CQuote& quote = item.second;
		const std::string& strSecurity = item.first;
		const auto rowIter = m_quoteRowIds.find(strSecurity);
		if (m_quoteRowIds.end() == rowIter)
		{
			_TyDataRowId rowId = m_nextQuoteRowId++;
			m_quoteRowIds.emplace(strSecurity, rowId);
			std::string strName;
			std::string strListingStatus;
			{
				std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
				m_pendingSecurityUpdates.erase(strSecurity);
				const auto nameIter = m_securityNames.find(strSecurity);
				if (m_securityNames.end() != nameIter)
				{
					strName = nameIter->second;
				}
				const auto statusIter = m_securityStatuses.find(strSecurity);
				if (m_securityStatuses.end() != statusIter)
				{
					strListingStatus = GetMarketStateString(statusIter->second);
				}
			}
			double fChange = quote.m_fLastPrice - quote.m_fPreClose;
			double fPercent = 0.0 == quote.m_fPreClose ? 0.0 : fChange * 100.0 / quote.m_fPreClose;
			writer.AddRow(rowId, { strSecurity, strName, quote.m_fLastPrice, fChange, fPercent, quote.m_fPreClose, quote.m_nVolume, std::string(quote.m_bStale ? "已延迟" : "交易中"), quote.m_nSequence, strListingStatus, MarketCategory(strSecurity) });
			continue;
		}
		_TyDataRowId rowId = rowIter->second;
		std::string strName;
		std::string strListingStatus;
		bool bMetadataChanged = false;
		{
			std::lock_guard<std::mutex> lock(m_mtx_pendingQuotes);
			bMetadataChanged = 0 != m_pendingSecurityUpdates.erase(strSecurity);
			if (bMetadataChanged)
			{
				const auto nameIter = m_securityNames.find(strSecurity);
				if (m_securityNames.end() != nameIter)
				{
					strName = nameIter->second;
				}
				const auto statusIter = m_securityStatuses.find(strSecurity);
				if (m_securityStatuses.end() != statusIter)
				{
					strListingStatus = GetMarketStateString(statusIter->second);
				}
			}
		}
		double fChange = quote.m_fLastPrice - quote.m_fPreClose;
		double fPercent = 0.0 == quote.m_fPreClose ? 0.0 : fChange * 100.0 / quote.m_fPreClose;
		if (bMetadataChanged)
		{
			writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Name), strName);
			writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::ListingStatus), strListingStatus);
			writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Market), MarketCategory(strSecurity));
		}
		writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::LastPrice), quote.m_fLastPrice);
		writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Change), fChange);
		writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Percent), fPercent);
		writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::PreClose), quote.m_fPreClose);
		writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Volume), quote.m_nVolume);
		writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Status), std::string(quote.m_bStale ? "已延迟" : "交易中"));
		writer.SetValue(rowId, static_cast<_TyDataColumnId>(MarketQuoteColumn::Sequence), quote.m_nSequence);
	}

	auto [view, changes] = writer.Commit();
	m_pump_quote_table.Notify(CQuoteTableEvent{ std::move(view), std::move(changes) });
}

void CHQMarketService::OnRequestReply(const CRequest& req)
{
	std::string strCmd = req.GetCmd();
	const _TyReqData& message = req.GetData();
	if ("query_sectors" == strCmd)
	{
		if (req.GetId() != m_nLatestSectorListRequest.load())
		{
			return;
		}
		CSectorListEvent ev;
		ev.m_nRequestId = req.GetId();
		ev.m_type = SectorType::industry;
		std::optional<std::pair<int, std::string>> errorInfo = req.GetErrorInfo();
		if (errorInfo.has_value() && (0 != errorInfo->first))
		{
			ev.m_strError = errorInfo->second.empty() ? "板块查询失败" : errorInfo->second;
		}
		else if (!message.has_sector_list())
		{
			ev.m_strError = "板块响应缺少数据";
		}
		else
		{
			const hqmarket::market::v1::SectorList& data = message.sector_list();
			ev.m_type = static_cast<SectorType>(data.type());
			ev.m_sectors.reserve(data.sectors_size());
			for (const auto& value : data.sectors())
			{
				ev.m_sectors.emplace_back(ParseSector(value, ev.m_type));
			}
			{
				std::unique_lock lock(m_mtx_sectors);
				m_sectors = ev.m_sectors;
			}
		}
		m_pump_sector_list.Notify(ev);
		return;
	}
	if ("query_sector_constituents" == strCmd)
	{
		if (req.GetId() != m_nLatestSectorConstituentsRequest.load())
		{
			return;
		}
		CSectorConstituentsEvent event;
		event.m_nRequestId = req.GetId();
		std::optional<std::pair<int, std::string>> errorInfo = req.GetErrorInfo();
		if (errorInfo.has_value() && (0 != errorInfo->first))
		{
			event.m_strError = errorInfo->second.empty() ? "成分股查询失败" : errorInfo->second;
		}
		else if (!message.has_sector_constituents())
		{
			event.m_strError = "成分股响应缺少数据";
		}
		else
		{
			const hqmarket::market::v1::SectorConstituents& data = message.sector_constituents();
			SectorType type = static_cast<SectorType>(data.type());
			event.m_sector = ParseSector(data.sector(), type);
			event.m_securities.reserve(data.securities_size());
			for (const auto& value : data.securities())
			{
				CSecurity security(value.security().symbol(), value.name(), ToExchange(value.security().exchange()), ParseMarketState(value.status()));
				if (!security.IsValid())
				{
					continue;
				}
				event.m_securities.emplace_back(security);
				RegisterSecurity(security);
			}
		}
		m_pump_sector_constituents.Notify(event);
		return;
	}
	if (("query_securities" == strCmd) && message.has_security_list())
	{
		const auto& data = message.security_list();
		CSecurityListEvent ev;
		ev.m_version = data.version();
		ev.m_securities.reserve(data.securities_size());
		for (const auto& v : data.securities())
		{
			std::string strExchange = ExchangeSuffix(v.security().exchange());
			if (strExchange.empty())
			{
				continue;
			}
			ev.m_securities.emplace_back(v.security().symbol(), v.name(), ToExchange(v.security().exchange()), ParseMarketState(v.status()));
		}
		{
			std::unique_lock lock(m_mtx_securities);
			m_securities = ev.m_securities;
		}
		for (const auto& security : ev.m_securities)
		{
			RegisterSecurity(security);
			if (CSession::InstanceRef().IsAuthenticated())
			{
				SubscribeQuote(security);
			}
		}
		m_pump_security_list.Notify(ev);
		return;
	}

	if (("depth" == strCmd) && message.has_depth())
	{
		const _TyDepthData& data = message.depth();
		CMarketDepth value;
		value.m_security = CSecurity(data.security().symbol(), ToExchange(data.security().exchange()));
		value.m_nExchangeTime = data.exchange_time_ms();
		value.m_bStale = data.stale();
		value.m_bids.reserve(data.bids_size());
		value.m_asks.reserve(data.asks_size());
		for (const auto& level : data.bids())
		{
			value.m_bids.emplace_back(CPriceLevel{ ScaledPrice(level.price(), level.price_scale()), level.volume() });
		}
		for (const auto& level : data.asks())
		{
			value.m_asks.emplace_back(CPriceLevel{ ScaledPrice(level.price(), level.price_scale()), level.volume() });
		}
		{
			std::unique_lock lock(m_mtx_depths);
			m_depths.insert_or_assign(value.m_security.String(), value);
		}
		m_pump_depth.Notify(value);
		return;
	}

	if (("query_response" == strCmd) || ("query_bars" == strCmd))
	{
		CMarketHistoryEvent event;
		{
			std::lock_guard<std::mutex> lock(m_mtx_historyRequests);
			auto requestIter = m_historyRequests.find(req.GetId());
			if (m_historyRequests.end() == requestIter)
			{
				return;
			}
			event = std::move(requestIter->second);
			m_historyRequests.erase(requestIter);
		}
		std::optional<std::pair<int, std::string>> errorInfo = req.GetErrorInfo();
		if (errorInfo.has_value() && (0 != errorInfo->first))
		{
			event.m_strError = errorInfo->second.empty() ? "历史行情查询失败" : errorInfo->second;
		}
		else if (!message.has_query_response())
		{
			event.m_strError = "历史行情响应缺少数据";
		}
		else
		{
			const _TyQueryResponse& data = message.query_response();
			MarketBarPeriod period = hqmarket::market::v1::CHANNEL_BAR_1M == data.channel() ? MarketBarPeriod::Minute : MarketBarPeriod::Day;
			if ((event.m_strSecurity != SecurityName(data.security())) || (event.m_period != period))
			{
				event.m_strError = "历史行情响应与请求不匹配";
			}
			else
			{
				event.m_bars.reserve(data.bars_size());
				for (const auto& bar : data.bars())
				{
					event.m_bars.emplace_back(CMarketBar{ bar.begin_time_ms(), ScaledPrice(bar.open_price(), bar.price_scale()), ScaledPrice(bar.high_price(), bar.price_scale()), ScaledPrice(bar.low_price(), bar.price_scale()), ScaledPrice(bar.close_price(), bar.price_scale()), bar.volume(), bar.turnover() });
				}
				std::unique_lock lock(m_mtx_history);
				m_history.insert_or_assign(HistoryKey(event.m_strSecurity, event.m_period), event.m_bars);
			}
		}
		m_pump_history.Notify(event);
		return;
	}

	if (("quote" != strCmd) || !message.has_quote())
	{
		return;
	}

	const _TyQuoteData& quote = message.quote();
	++m_runtimeMetrics.m_nQuoteReceived;
	std::uint64_t sequence = ParseSequence(req.GetReturnData());
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
	value.m_security = CSecurity(quote.security().symbol(), ToExchange(quote.security().exchange()));
	value.m_nSequence = sequence;
	value.m_fLastPrice = static_cast<double>(quote.last_price()) / fScale;
	value.m_fPreClose = static_cast<double>(quote.pre_close()) / fScale;
	value.m_nVolume = quote.volume();
	value.m_bStale = quote.stale();
	++m_runtimeMetrics.m_nQuoteAccepted;
	EnqueueQuote(value);
	{
		std::unique_lock lock(m_mtx_quotes);
		m_quotes.insert_or_assign(value.m_security.String(), value);
	}

	m_pump_quote.Notify(value);
}
