#include "CDBEngine.h"

CDBEngine::CDBEngine()
{

}

CDBEngine::~CDBEngine()
{

}

db::CSQLite3* CDBEngine::Connect(sqlite ty)
{
	return dynamic_cast<db::CSQLite3*>(db::CODBC::InstancePtr()->Connect(db::database::sqlite, GetConnectParam(ty)));
}

db::CMySQL* CDBEngine::Connect(mysql ty)
{
	return dynamic_cast<db::CMySQL*>(db::CODBC::InstancePtr()->Connect(db::database::mysql, GetConnectParam(ty)));
}

db::COracle* CDBEngine::Connect(oracle ty)
{
	return dynamic_cast<db::COracle*>(db::CODBC::InstancePtr()->Connect(db::database::oracle, GetConnectParam(ty)));
}

int CDBEngine::Close(sqlite ty)
{
	return db::CODBC::InstancePtr()->Release(db::database::sqlite, GetConnectParam(ty));
}

int CDBEngine::Close(mysql ty)
{
	return db::CODBC::InstancePtr()->Release(db::database::mysql, GetConnectParam(ty));
}

int CDBEngine::Close(oracle ty)
{
	return db::CODBC::InstancePtr()->Release(db::database::oracle, GetConnectParam(ty));
}

int CDBEngine::Close(db::database ty)
{
	return db::CODBC::InstancePtr()->Release(db::database::oracle);
}

void CDBEngine::InitConnectParam()
{
	
}

std::string CDBEngine::GetConnectParam(sqlite ty)
{
	const auto& mIter = m_sqlite.find(ty);
	if (mIter == m_sqlite.end())
	{
		return "";
	}
	return mIter->second;
}


std::string CDBEngine::GetConnectParam(mysql ty)
{
	const auto& mIter = m_mysql.find(ty);
	if (mIter == m_mysql.end())
	{
		return "";
	}
	return mIter->second;
}

std::string CDBEngine::GetConnectParam(oracle ty)
{
	const auto& mIter = m_oracle.find(ty);
	if (mIter == m_oracle.end())
	{
		return "";
	}
	return mIter->second;
}