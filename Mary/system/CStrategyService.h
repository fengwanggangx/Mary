#ifndef MARY_SERVER_CSTRATEGYSERVICE_H
#define MARY_SERVER_CSTRATEGYSERVICE_H

#include "../common/ISingleton.h"
#include "../request/request.pb.h"

#include <functional>
#include <mutex>
#include <string>
#include <vector>

class CRequest;

class CStrategyService final : public ISingleton<CStrategyService>
{
	DECLARE_SINGLE_DFAULT(CStrategyService)

  public:
	using StrategyList = std::vector<request::StrategyInfo>;
	using QueryHandler = std::function<void(const StrategyList&, const std::string&)>;
	using OperationHandler = std::function<void(const std::string&, bool, const request::StrategyInfo&, const std::string&)>;

	void Initialize();
	void SetQueryHandler(QueryHandler&& handler);
	void SetOperationHandler(OperationHandler&& handler);
	bool QueryStrategies();
	bool AddStrategy(const request::StrategyInfo& strategy);
	bool ModifyStrategy(const request::StrategyInfo& strategy);
	bool DeleteStrategy(std::uint64_t id);

  private:
	void OnResponse(const CRequest& response);

  private:
	bool m_bInitialized{ false };
	std::mutex m_mtx_handlers;
	QueryHandler m_queryHandler;
	OperationHandler m_operationHandler;
};

#endif
