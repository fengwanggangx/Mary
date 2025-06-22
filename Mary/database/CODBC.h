#ifndef __CODBC_H__
#define __CODBC_H__

#include <unordered_map>
#include <mutex>
#include "./common/ISingleton.h"

namespace db
{
	enum class database
	{
		sqlite = 0,
		mysql,
		oracle
	};



	class IDataBase;
	class CODBC final : public ISingleton<CODBC>
	{
		DECLARE_SINGLE_DFAULT(CODBC)
	public:
		IDataBase* Connect(db::database ty, const std::string& strParam);
		int Release();
		int Release(db::database ty);
		int Release(db::database ty, const std::string& strParam);
	private:
		std::mutex m_mtx;
		std::unordered_map<db::database, std::unordered_map<std::size_t, IDataBase*>> m_database;

	};

}
#endif
