#ifndef __CSQLITE3_H__
#define __CSQLITE3_H__

#include <sqlite3.h>
#include "IDataBase.h"

namespace db
{
	class CSQLite3 : public IDataBase
	{
	public:
		CSQLite3();
		~CSQLite3();

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
		sqlite3* m_pDB{ nullptr };
	};
}
#endif
