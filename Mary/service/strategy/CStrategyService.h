#ifndef MARY_SERVER_CSTRATEGYSERVICE_H
#define MARY_SERVER_CSTRATEGYSERVICE_H

#include "../../common/ISingleton.h"
#include "../../request/request.pb.h"
#include "../../basic/TMessagePump.h"

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

class CRequest;

class CStrategyService final : public ISingleton<CStrategyService>
{
		DECLARE_SINGLE_DFAULT(CStrategyService)

	public:
		using _TyStrategyList = std::vector<request::StrategyInfo>;
		using _TyQueryHandler = std::function<void(const _TyStrategyList&, const std::string&)>;
		using _TyOperationHandler = std::function<void(const std::string&, bool, const request::StrategyInfo&, const std::string&)>;

		void Initialize();
		void SetQueryHandler(_TyQueryHandler&& handler);
		_TyCallbackId AddQueryHandler(_TyQueryHandler&& handler);
		void RemoveQueryHandler(_TyCallbackId nToken);
		void SetOperationHandler(_TyOperationHandler&& handler);
		bool QueryStrategies();
		bool AddStrategy(const request::StrategyInfo& strategy);
		bool ModifyStrategy(const request::StrategyInfo& strategy);
		bool DeleteStrategy(std::uint64_t id);
		_TyStrategyList GetStrategies() const;
		bool FindStrategy(std::uint64_t id, request::StrategyInfo& result) const;
		bool IsCacheValid() const;
		static bool ValidateStrategy(const request::StrategyInfo& strategy, std::string& error);

	private:
		void OnRequestReply(const CRequest& response);

	private:
		std::once_flag m_initializeFlag;
		std::mutex m_mtx_handlers;
		_TyQueryHandler m_queryHandler;
		TMessagePump<std::pair<_TyStrategyList, std::string>> m_queryPump;
		_TyOperationHandler m_operationHandler;
		mutable std::shared_mutex m_mtx_strategies;
		_TyStrategyList m_strategies;
		bool m_bCacheValid{ false };
};

#endif
