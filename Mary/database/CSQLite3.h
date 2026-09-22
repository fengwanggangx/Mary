#ifndef __CSQLITE3_H__
#define __CSQLITE3_H__

#include "IDataBase.h"
#include <cstdint>
#include <variant>

namespace db
{
	class CSQLite3 : public IDataBase
	{
	  public:
		using _TySqlValue = std::variant<std::nullptr_t, std::int64_t, std::string, std::vector<std::uint8_t>>;
		using _TySqlParameters = std::vector<_TySqlValue>;
		using _TySqlRows = std::vector<std::vector<_TySqlValue>>;

	  public:
		CSQLite3();
		~CSQLite3();

	  public:
		int Connect(const CConnectParam& param) override;
		int Close() override;

		int ExecUpdate(const std::string& strSQL) override;
		const _TyTableInfo& ExecQuery(const std::string& strSQL) override;
		int ExecUpdatePrepared(const std::string& strSQL, const _TySqlParameters& parameters);
		int ExecQueryPrepared(const std::string& strSQL, const _TySqlParameters& parameters, _TySqlRows& rows);

		bool BeginTransaction() override;
		bool EndTransaction() override;
		bool RollBackTransaction() override;

	  private:
		void* m_pDB{ nullptr };
	};
} // namespace db
#endif
