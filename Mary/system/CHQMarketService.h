#ifndef MARY_SYSTEM_CHQMARKETSERVICE_H
#define MARY_SYSTEM_CHQMARKETSERVICE_H

#include "../common/ISingleton.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

struct SessionResponse;

struct CQuote
{
	std::string m_strSecurity;
	double m_fLastPrice{ 0.0 };
	double m_fPreClose{ 0.0 };
	std::int64_t m_nVolume{ 0 };
	bool m_bStale{ false };
};

class CHQMarketService final : public ISingleton<CHQMarketService>
{
	DECLARE_SINGLE_DFAULT(CHQMarketService)

public:
	using QuoteHandler = std::function<void(const CQuote&)>;

	void Initialize();
	void SetQuoteHandler(QuoteHandler&& handler);
	bool SubscribeQuote(const std::string& strSecurity);
	bool UnsubscribeQuote(const std::string& strSecurity);
	bool FindQuote(const std::string& strSecurity, CQuote& result) const;

private:
	void OnResponse(const SessionResponse& response);

private:
	bool m_bInitialized{ false };
	mutable std::mutex m_mtx_quotes;
	std::unordered_map<std::string, CQuote> m_quotes;
	std::mutex m_mtx_handler;
	QuoteHandler m_quoteHandler;
};

#endif
