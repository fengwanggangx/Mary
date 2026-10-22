#pragma once
#include "../../basic/TMessagePump.h"
#include "../../common/ISingleton.h"
#include "../../request/request.h"
#include "../../request/request.pb.h"
#include <mutex>
#include <unordered_map>
#include <vector>
class CTradingAccountService final : public ISingleton<CTradingAccountService>
{
	DECLARE_SINGLE_DFAULT(CTradingAccountService)
	public:
	struct CResult
	{
		std::uint64_t m_requestId{ 0 };
		std::uint64_t m_accountId{ 0 };
		std::uint64_t m_generation{ 0 };
		std::string m_command;
		std::string m_error;
		request::RequestData m_data;
	};
	using _TyHandler = std::function<void(const CResult&)>;
	void Initialize();
	_TyCallbackId Subscribe(_TyHandler&& handler);
	void Unsubscribe(_TyCallbackId token);
	std::uint64_t Send(const std::string& command, std::uint64_t accountId = 0, const std::unordered_map<std::string, std::string>& extra = {});
	std::vector<request::TradeAccountRecord> GetAccounts() const;
	bool IsSupported() const;
	std::uint64_t GetGeneration() const;

	private:
	void OnResponse(const CRequest& response);
	struct CPending
	{
		std::uint64_t m_accountId{ 0 };
		std::string m_command;
	};
	std::once_flag m_initializeFlag;
	mutable std::mutex m_mtx_state;
	std::unordered_map<std::uint64_t, CPending> m_pending;
	std::vector<request::TradeAccountRecord> m_accounts;
	bool m_supported{ true };
	std::uint64_t m_generation{ 0 };
	TMessagePump<CResult> m_results;
};
