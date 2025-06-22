#ifndef __CORACLE_H__
#define __CORACLE_H__

#include <sqlite3.h>
#include "IDataBase.h"

namespace db
{
	class COracle : public IDataBase
	{
	public:
		COracle();
		~COracle();

	public:
		int Connect(const std::string& strFile) override;
		int Close() override;

		const std::vector<std::vector<std::string>>& ExecQuery(const std::string& strSQL) override;
		int ExecUpdate(const std::string& strSQL) override;

		static int GetLastErrCode();
		static std::string GetLastErrMsg();
	public:
		static std::string GetRetCodeText(int nCode);

	private:
		static void ClearLastErr();

		static void SetLastErrCode(int nErr);
		static void SetLastErrMsg(const std::string& strErr);
		static void SetLastErr(int nErr, const std::string& strErr);

		static int& GetErrCodeRef();
		static std::string& GetErrMsgRef();
	private:
		void* m_pDB{ nullptr };
	};
}
#endif
