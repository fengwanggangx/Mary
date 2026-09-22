#include "CBootLoader.h"
#include "../database/CDBEngine.h"
#include "../database/IDataBase.h"
#include "../network/common_net.h"
#include <ini/CINIHandler.h>

CBootLoader::CBootLoader() = default;

CBootLoader::~CBootLoader()
{
	Finalize();
}

bool CBootLoader::Initialize()
{
	if (m_bInitialized)
	{
		return true;
	}

	m_exec = std::filesystem::current_path();

	m_nErrorCode = 0;
	m_strLastError.clear();
	if (!net::EnvInitialize())
	{
		m_nErrorCode = 1;
		m_strLastError = "Failed to enable libevent thread support";
		return false;
	}

	ini::CINIHandler& hIni = ini::CINIHandler::InstanceRef();
	int nSqlitePoolSize = hIni.GetValue(ini::Config::System, "Sqlite", "pool_size", -1);
	std::string strSqliteDB = hIni.GetValue(ini::Config::System, "Sqlite", "db_name", "");
	if ((nSqlitePoolSize <= 0) || strSqliteDB.empty())
	{
		m_nErrorCode = 2;
		m_strLastError = "Sqlite parameters error";
		return false;
	}

	db::CConnectParam dbParam("", 0, "", "", (m_exec / strSqliteDB).string(), "");
	if (0 != CDBEngine::InstanceRef().Initialize(db::em_database::sqlite, dbParam, 4))
	{
		m_nErrorCode = 3;
		m_strLastError = "Sqlite initialization failed";
		return false;
	}
	db::_TyDBPtr db = CDBEngine::InstanceRef().GetDBPtr(db::em_database::sqlite);
	if ((nullptr == db) || (0 != db->ExecSqlFile(m_exec / "sql" / "table_create.sql")))
	{
		m_nErrorCode = 4;
		m_strLastError = "Failed to initialize Sqlite tables";
		return false;
	}

	m_bInitialized = true;
	return true;
}

bool CBootLoader::Run()
{
	if (!m_bInitialized)
	{
		m_nErrorCode = 4;
		m_strLastError = "Boot loader is not initialized";
		return false;
	}
	return true;
}

void CBootLoader::Stop()
{
}

void CBootLoader::Finalize()
{
	if (!m_bInitialized)
	{
		return;
	}

	Stop();
	CDBEngine::InstanceRef().Close(db::em_database::sqlite);
	m_bInitialized = false;
	net::EnvCleanup();
}

const std::string& CBootLoader::GetLastError() const
{
	return m_strLastError;
}

int CBootLoader::GetErrorCode() const
{
	return m_nErrorCode;
}
