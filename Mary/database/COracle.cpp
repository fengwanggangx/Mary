#include "COracle.h"
#include <unordered_map>

namespace db
{
	COracle::COracle()
	{

	}

	COracle::~COracle()
	{
		Close();
	}

	int COracle::Connect(const std::string& strFile)
	{
		return 0;
	}

	int COracle::Close()
	{
		m_pDB = nullptr;
		return 0;
	}

	const std::vector<std::vector<std::string>>& COracle::ExecQuery(const std::string& strSQL)
	{
		ClearLastErr();
		thread_local std::vector<std::vector<std::string>> s_data;
		s_data.clear();
		return s_data;
	}

	int COracle::ExecUpdate(const std::string& strSQL)
	{
		ClearLastErr();
		return 0;
	}

	void COracle::SetLastErrCode(int nErr)
	{
		GetErrCodeRef() = nErr;
	}

	void COracle::SetLastErrMsg(const std::string& strErr)
	{
		GetErrMsgRef() = strErr;
	}

	void COracle::SetLastErr(int nErr, const std::string& strErr)
	{
		GetErrCodeRef() = nErr;
		GetErrMsgRef() = strErr;
	}

	void COracle::ClearLastErr()
	{
		GetErrCodeRef() = SQLITE_OK;
		GetErrMsgRef().clear();
	}

	int& COracle::GetErrCodeRef()
	{
		thread_local int s_nErrCode{ 0 };
		return s_nErrCode;
	}

	std::string& COracle::GetErrMsgRef()
	{
		thread_local std::string s_nErrCode{ 0 };
		return s_nErrCode;
	}

	int COracle::GetLastErrCode()
	{
		int nErr = GetErrCodeRef();
		GetErrCodeRef() = 0;
		return nErr;
	}

	std::string COracle::GetLastErrMsg()
	{
		std::string strErr = GetErrMsgRef();;
		GetErrMsgRef().clear();
		return strErr;
	}

	std::string COracle::GetRetCodeText(int nCode)
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