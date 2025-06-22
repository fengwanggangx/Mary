#include "CODBC.h"
#include <unordered_map>
#include "CSQLite3.h"
#include "IDataBase.h"


namespace db
{
	IDataBase* CreateDB(db::database ty)
	{
		switch (ty)
		{
		case db::database::sqlite:
			return new CSQLite3();
		case db::database::mysql:
			break;
		case db::database::oracle:
			break;
		default:
			break;
		}
		return nullptr;
	}

	std::size_t FmtConnectKey(db::database ty, const std::string& strParam)
	{
		std::size_t key = std::hash<int>()((int)ty) + std::hash<std::string>()(strParam);
		return key;
	}

	IDataBase* CODBC::Connect(db::database ty, const std::string& strParam)
	{
		std::size_t key = FmtConnectKey(ty, strParam);
		std::lock_guard<std::mutex> lck(m_mtx);
		const auto& mIter = m_database[ty].find(key);
		if (mIter != m_database[ty].end())
		{
			return mIter->second;
		}
		IDataBase* pDB = CreateDB(ty);
		if (nullptr == pDB)
		{
			return nullptr;
		}
		if (0 != pDB->Connect(strParam))
		{
			delete pDB;
			pDB = nullptr;
			return nullptr;
		}
		m_database[ty][key] = pDB;
		return pDB;
	}

	int CODBC::Release()
	{
		std::lock_guard<std::mutex> lck(m_mtx);
		for (const auto& data : m_database)
		{
			for (const auto& item : data.second)
			{
				if (nullptr != item.second)
				{
					item.second->Close();
					delete item.second;
				}
			}
		}
		m_database.clear();
		return 0;
	}

	int CODBC::Release(db::database ty)
	{
		std::lock_guard<std::mutex> lck(m_mtx);
		for (const auto& item : m_database[ty])
		{
			if (nullptr != item.second)
			{
				item.second->Close();
				delete item.second;
			}
		}
		m_database.erase(ty);
		return 0;
	}

	int CODBC::Release(db::database ty, const std::string& strParam)
	{
		std::size_t key = FmtConnectKey(ty, strParam);
		std::lock_guard<std::mutex> lck(m_mtx);
		const auto& mIter = m_database[ty].find(key);
		if (mIter == m_database[ty].end())
		{
			return 0;
		}
		if (nullptr != mIter->second)
		{
			mIter->second->Close();
			delete mIter->second;
		}
		m_database[ty].erase(mIter);
		return 0;
	}

	CODBC::CODBC()
	{

	}
	CODBC::~CODBC()
	{

	}
}