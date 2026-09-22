#include "CSQLite3.h"
#include <memory>
#include <sqlite3.h>
#include <type_traits>
#include <utility>

namespace db
{
	namespace
	{
		int BindParameters(sqlite3_stmt* pStatement, const CSQLite3::_TySqlParameters& parameters)
		{
			for (std::size_t i = 0; i < parameters.size(); ++i)
			{
				int nIndex = static_cast<int>(i + 1);
				int nResult = std::visit([pStatement, nIndex](const auto& value)
										 {
											 using _TyValue = std::decay_t<decltype(value)>;
											 if constexpr (std::is_same_v<_TyValue, std::nullptr_t>)
											 {
												 return sqlite3_bind_null(pStatement, nIndex);
											 }
											 else if constexpr (std::is_same_v<_TyValue, std::int64_t>)
											 {
												 return sqlite3_bind_int64(pStatement, nIndex, value);
											 }
											 else if constexpr (std::is_same_v<_TyValue, std::string>)
											 {
												 return sqlite3_bind_text(pStatement, nIndex, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
											 }
											 else
											 {
												 return sqlite3_bind_blob(pStatement, nIndex, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
											 }
										 },
										 parameters[i]);
				if (SQLITE_OK != nResult)
				{
					return nResult;
				}
			}
			return SQLITE_OK;
		}
	} // namespace

	CSQLite3::CSQLite3()
	{
	}

	CSQLite3::~CSQLite3()
	{
		Close();
	}

	int CSQLite3::Connect(const CConnectParam& param)
	{
		if (nullptr != m_pDB)
		{
			return SQLITE_OK;
		}

		sqlite3* pDB = nullptr;
		int nRet = sqlite3_open_v2(param.m_strDataBase.c_str(), &pDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
		if (SQLITE_OK != nRet)
		{
			if (nullptr != pDB)
			{
				sqlite3_close(pDB);
			}
			return nRet;
		}

		m_pDB = pDB;
		return SQLITE_OK;
	}

	int CSQLite3::Close()
	{
		if (nullptr == m_pDB)
		{
			return SQLITE_OK;
		}

		int nRet = sqlite3_close(static_cast<sqlite3*>(m_pDB));
		if (SQLITE_OK == nRet)
		{
			m_pDB = nullptr;
		}
		return nRet;
	}

	int CSQLite3::ExecUpdate(const std::string& strSQL)
	{
		if (nullptr == m_pDB)
		{
			return SQLITE_MISUSE;
		}
		return sqlite3_exec(static_cast<sqlite3*>(m_pDB), strSQL.c_str(), nullptr, nullptr, nullptr);
	}

	const _TyTableInfo& CSQLite3::ExecQuery(const std::string& strSQL)
	{
		static _TyTableInfo table;
		table = {};
		if (nullptr == m_pDB)
		{
			return table;
		}

		sqlite3_stmt* pStatement = nullptr;
		int nRet = sqlite3_prepare_v2(static_cast<sqlite3*>(m_pDB), strSQL.c_str(), -1, &pStatement, nullptr);
		std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(pStatement, sqlite3_finalize);
		if (SQLITE_OK != nRet)
		{
			return table;
		}

		int nCols = sqlite3_column_count(statement.get());
		table.first.reserve(static_cast<std::size_t>(nCols));
		for (int i = 0; i < nCols; ++i)
		{
			table.first.emplace_back();
			CColumnInfo& column = table.first.back();
			column.m_uId = static_cast<unsigned int>(i);
			column.m_strName = sqlite3_column_name(statement.get(), i);
		}

		while (SQLITE_ROW == sqlite3_step(statement.get()))
		{
			table.second.emplace_back();
			auto& row = table.second.back();
			row.reserve(static_cast<std::size_t>(nCols));
			for (int i = 0; i < nCols; ++i)
			{
				const unsigned char* pszValue = sqlite3_column_text(statement.get(), i);
				row.emplace_back(nullptr == pszValue ? "" : reinterpret_cast<const char*>(pszValue));
			}
		}
		return table;
	}

	int CSQLite3::ExecUpdatePrepared(const std::string& strSQL, const _TySqlParameters& parameters)
	{
		if (nullptr == m_pDB)
		{
			return SQLITE_MISUSE;
		}
		sqlite3_stmt* pStatement = nullptr;
		int nResult = sqlite3_prepare_v2(static_cast<sqlite3*>(m_pDB), strSQL.c_str(), -1, &pStatement, nullptr);
		std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(pStatement, sqlite3_finalize);
		if (SQLITE_OK != nResult)
		{
			return nResult;
		}
		nResult = BindParameters(statement.get(), parameters);
		if (SQLITE_OK != nResult)
		{
			return nResult;
		}
		nResult = sqlite3_step(statement.get());
		return SQLITE_DONE == nResult ? SQLITE_OK : nResult;
	}

	int CSQLite3::ExecQueryPrepared(const std::string& strSQL, const _TySqlParameters& parameters, _TySqlRows& rows)
	{
		rows.clear();
		if (nullptr == m_pDB)
		{
			return SQLITE_MISUSE;
		}
		sqlite3_stmt* pStatement = nullptr;
		int nResult = sqlite3_prepare_v2(static_cast<sqlite3*>(m_pDB), strSQL.c_str(), -1, &pStatement, nullptr);
		std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(pStatement, sqlite3_finalize);
		if (SQLITE_OK != nResult)
		{
			return nResult;
		}
		nResult = BindParameters(statement.get(), parameters);
		if (SQLITE_OK != nResult)
		{
			return nResult;
		}
		int nColumns = sqlite3_column_count(statement.get());
		while (SQLITE_ROW == (nResult = sqlite3_step(statement.get())))
		{
			std::vector<_TySqlValue>& row = rows.emplace_back();
			row.reserve(static_cast<std::size_t>(nColumns));
			for (int i = 0; i < nColumns; ++i)
			{
				int nType = sqlite3_column_type(statement.get(), i);
				if (SQLITE_INTEGER == nType)
				{
					row.emplace_back(static_cast<std::int64_t>(sqlite3_column_int64(statement.get(), i)));
				}
				else if (SQLITE_BLOB == nType)
				{
					const std::uint8_t* pData = static_cast<const std::uint8_t*>(sqlite3_column_blob(statement.get(), i));
					int nSize = sqlite3_column_bytes(statement.get(), i);
					row.emplace_back(nullptr == pData ? std::vector<std::uint8_t>() : std::vector<std::uint8_t>(pData, pData + nSize));
				}
				else if (SQLITE_NULL == nType)
				{
					row.emplace_back(nullptr);
				}
				else
				{
					const char* pText = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), i));
					row.emplace_back(nullptr == pText ? std::string() : std::string(pText));
				}
			}
		}
		return SQLITE_DONE == nResult ? SQLITE_OK : nResult;
	}

	bool CSQLite3::BeginTransaction()
	{
		return SQLITE_OK == ExecUpdate("BEGIN IMMEDIATE");
	}

	bool CSQLite3::EndTransaction()
	{
		return SQLITE_OK == ExecUpdate("COMMIT");
	}

	bool CSQLite3::RollBackTransaction()
	{
		return SQLITE_OK == ExecUpdate("ROLLBACK");
	}
} // namespace db
