#include "CMySQL.h"
#include <unordered_map>

namespace db
{
	CMySQL::CMySQL()
	{

	}

	CMySQL::~CMySQL()
	{
		Close();
	}

	int CMySQL::Connect(const std::string& strFile)
	{
		return 0;
	}

	int CMySQL::Close()
	{
		m_pDB = nullptr;
		return 0;
	}

	const std::vector<std::vector<std::string>>& CMySQL::ExecQuery(const std::string& strSQL)
	{
		ClearLastErr();
		thread_local std::vector<std::vector<std::string>> s_data;
		s_data.clear();
		return s_data;
	}

	int CMySQL::ExecUpdate(const std::string& strSQL)
	{
		ClearLastErr();
		return 0;
	}

	void CMySQL::SetLastErrCode(int nErr)
	{
		GetErrCodeRef() = nErr;
	}

	void CMySQL::SetLastErrMsg(const std::string& strErr)
	{
		GetErrMsgRef() = strErr;
	}

	void CMySQL::SetLastErr(int nErr, const std::string& strErr)
	{
		GetErrCodeRef() = nErr;
		GetErrMsgRef() = strErr;
	}

	void CMySQL::ClearLastErr()
	{
		GetErrCodeRef() = SQLITE_OK;
		GetErrMsgRef().clear();
	}

	int& CMySQL::GetErrCodeRef()
	{
		thread_local int s_nErrCode{ 0 };
		return s_nErrCode;
	}

	std::string& CMySQL::GetErrMsgRef()
	{
		thread_local std::string s_nErrCode{ 0 };
		return s_nErrCode;
	}

	int CMySQL::GetLastErrCode()
	{
		int nErr = GetErrCodeRef();
		GetErrCodeRef() = 0;
		return nErr;
	}

	std::string CMySQL::GetLastErrMsg()
	{
		std::string strErr = GetErrMsgRef();;
		GetErrMsgRef().clear();
		return strErr;
	}

	std::string CMySQL::GetRetCodeText(int nCode)
	{
		static std::unordered_map<int, std::string> s_code
		{
			
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