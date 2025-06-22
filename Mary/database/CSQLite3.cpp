#include "CSQLite3.h"
#include <unordered_map>

namespace db
{
	CSQLite3::CSQLite3()
	{

	}

	CSQLite3::~CSQLite3()
	{
		Close();
	}

	int CSQLite3::Connect(const std::string& strFile)
	{
		int nRet = sqlite3_open(strFile.c_str(), &m_pDB);

		if (nRet != SQLITE_OK)
		{
			const char* szError = sqlite3_errmsg(m_pDB);
			m_pDB = nullptr;
		}
		return nRet;
	}

	int CSQLite3::Close()
	{
		if (nullptr == m_pDB)
		{
			return SQLITE_OK;
		}
		int nRet = sqlite3_close(m_pDB);
		if (nRet != SQLITE_OK)
		{
			const char* szError = sqlite3_errmsg(m_pDB);
			m_pDB = nullptr;
			return nRet;
		}
		m_pDB = nullptr;
		return nRet;
	}

	const std::vector<std::vector<std::string>>& CSQLite3::ExecQuery(const std::string& strSQL)
	{
		ClearLastErr();
		thread_local std::vector<std::vector<std::string>> s_data;
		s_data.clear();
		if (nullptr == m_pDB)
		{
			return s_data;
		}
		const char* szTail = 0;
		sqlite3_stmt* pStmt = nullptr;

		int nRet = sqlite3_prepare_v2(m_pDB, strSQL.c_str(), -1, &pStmt, &szTail);
		if (nRet != SQLITE_OK)
		{
			SetLastErr(nRet, sqlite3_errmsg(m_pDB));
			return s_data;
		}
		int nColumns = sqlite3_column_count(pStmt);
		while (sqlite3_step(pStmt) == SQLITE_ROW)
		{
			std::vector<std::string> row(nColumns);
			for (int i = 0; i < nColumns; ++i)
			{
				row.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(pStmt, i)));
			}
			s_data.emplace_back(std::move(row));
		}
		nRet = sqlite3_finalize(pStmt);
		if (nRet != SQLITE_OK)
		{
			SetLastErr(nRet, sqlite3_errmsg(m_pDB));
			return s_data;
		}
		return s_data;
	}

	int CSQLite3::ExecUpdate(const std::string& strSQL)
	{
		ClearLastErr();
		if (nullptr == m_pDB)
		{
			return 0;
		}
		char* szError = 0;
		int nRet = sqlite3_exec(m_pDB, strSQL.c_str(), 0, 0, &szError);
		if (nRet != SQLITE_OK)
		{
			SetLastErr(nRet, sqlite3_errmsg(m_pDB));
			return 0;
		}
		return sqlite3_changes(m_pDB);
	}

	void CSQLite3::SetLastErrCode(int nErr)
	{
		GetErrCodeRef() = nErr;
	}

	void CSQLite3::SetLastErrMsg(const std::string& strErr)
	{
		GetErrMsgRef() = strErr;
	}

	void CSQLite3::SetLastErr(int nErr, const std::string& strErr)
	{
		GetErrCodeRef() = nErr;
		GetErrMsgRef() = strErr;
	}

	void CSQLite3::ClearLastErr()
	{
		GetErrCodeRef() = SQLITE_OK;
		GetErrMsgRef().clear();
	}

	int& CSQLite3::GetErrCodeRef()
	{
		thread_local int s_nErrCode{ 0 };
		return s_nErrCode;
	}

	std::string& CSQLite3::GetErrMsgRef()
	{
		thread_local std::string s_nErrCode{ 0 };
		return s_nErrCode;
	}

	int CSQLite3::GetLastErrCode()
	{
		int nErr = GetErrCodeRef();
		GetErrCodeRef() = 0;
		return nErr;
	}

	std::string CSQLite3::GetLastErrMsg()
	{
		std::string strErr = GetErrMsgRef();;
		GetErrMsgRef().clear();
		return strErr;
	}

	std::string CSQLite3::GetRetCodeText(int nCode)
	{
		static std::unordered_map<int, std::string> s_code
		{
			{SQLITE_OK, "SQLITE_OK"},
			{SQLITE_ERROR, "SQLITE_ERROR"},
			{SQLITE_INTERNAL, "SQLITE_INTERNAL"},
			{SQLITE_PERM, "SQLITE_PERM"},
			{SQLITE_ABORT, "SQLITE_ABORT"},
			{SQLITE_BUSY, "SQLITE_BUSY"},
			{SQLITE_LOCKED, "SQLITE_LOCKED"},
			{SQLITE_NOMEM, "SQLITE_NOMEM"},
			{SQLITE_READONLY, "SQLITE_READONLY"},
			{SQLITE_INTERRUPT, "SQLITE_INTERRUPT"},
			{SQLITE_IOERR, "SQLITE_IOERR"},
			{SQLITE_CORRUPT, "SQLITE_CORRUPT"},
			{SQLITE_NOTFOUND, "SQLITE_NOTFOUND"},
			{SQLITE_FULL, "SQLITE_FULL"},
			{SQLITE_CANTOPEN, "SQLITE_CANTOPEN"},
			{SQLITE_PROTOCOL, "SQLITE_PROTOCOL"},
			{SQLITE_EMPTY, "SQLITE_EMPTY"},
			{SQLITE_SCHEMA, "SQLITE_SCHEMA"},
			{SQLITE_TOOBIG, "SQLITE_TOOBIG"},
			{SQLITE_CONSTRAINT, "SQLITE_CONSTRAINT"},
			{SQLITE_MISMATCH, "SQLITE_MISMATCH"},
			{SQLITE_MISUSE, "SQLITE_MISUSE"},
			{SQLITE_NOLFS, "SQLITE_NOLFS"},
			{SQLITE_AUTH, "SQLITE_AUTH"},
			{SQLITE_FORMAT, "SQLITE_FORMAT"},
			{SQLITE_RANGE, "SQLITE_RANGE"},
			{SQLITE_ROW, "SQLITE_ROW"},
			{SQLITE_DONE, "SQLITE_DONE"}
		};

		static std::string s_default = "UNKNOWN_CODE";
		const auto& mIter = s_code.find(nCode);
		if (mIter == s_code.end())
		{
			return s_default;
		}
		return mIter->second;
	}
}