#include "CHQMarketService.h"

#include "CSession.h"
#include "../request/v1/market.pb.h"

#include <cmath>
#include <utility>

CHQMarketService::CHQMarketService() = default;

CHQMarketService::~CHQMarketService() = default;

void CHQMarketService::Initialize()
{
	if (m_bInitialized)
	{
		return;
	}
	m_bInitialized = true;
	CSession::InstanceRef().RegisterResponseHandler([this](const SessionResponse& response)
	{
		OnResponse(response);
	});
}

void CHQMarketService::SetQuoteHandler(QuoteHandler&& handler)
{
	std::lock_guard<std::mutex> lock(m_mtx_handler);
	m_quoteHandler = std::move(handler);
}

bool CHQMarketService::SubscribeQuote(const std::string& strSecurity)
{
	request::RequestParameters parameters
	{
		{ "security", strSecurity },
		{ "channel", "quote" }
	};
	return CSession::InstanceRef().Subscribe("watchlist:" + strSecurity, parameters);
}

bool CHQMarketService::UnsubscribeQuote(const std::string& strSecurity)
{
	request::RequestParameters parameters
	{
		{ "security", strSecurity },
		{ "channel", "quote" }
	};
	return CSession::InstanceRef().Unsubscribe("watchlist:" + strSecurity, parameters);
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

void CHQMarketService::OnResponse(const SessionResponse& response)
{
	if (("quote" != response.m_cmd) || !response.m_message.has_quote())
	{
		return;
	}

	const hqmarket::market::v1::QuoteData& quote = response.m_message.quote();
	double fScale = std::pow(10.0, quote.price_scale());
	if (0.0 >= fScale)
	{
		return;
	}

	CQuote value;
	value.m_strSecurity = quote.instrument().symbol();
	value.m_fLastPrice = static_cast<double>(quote.last_price()) / fScale;
	value.m_fPreClose = static_cast<double>(quote.pre_close()) / fScale;
	value.m_nVolume = quote.volume();
	value.m_bStale = quote.stale();
	{
		std::lock_guard<std::mutex> lock(m_mtx_quotes);
		m_quotes.insert_or_assign(value.m_strSecurity, value);
	}

	QuoteHandler handler;
	{
		std::lock_guard<std::mutex> lock(m_mtx_handler);
		handler = m_quoteHandler;
	}
	if (nullptr != handler)
	{
		handler(value);
	}
}
