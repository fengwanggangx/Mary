#ifndef __IDB_H__
#define __IDB_H__

#include <string>
#include <vector>


namespace db
{
	class IDataBase
	{
	public:
		virtual int Connect(const std::string& strFile) = 0;
		virtual int Close() = 0;

		virtual int ExecUpdate(const std::string& strSQL) = 0;
		virtual const std::vector<std::vector<std::string>>& ExecQuery(const std::string& strSQL) = 0;
	};
}
#endif
