#include "CBootLoader.h"
#include "../network/CTcpClient.h"
#include "../ini/CINIHandler.h"
#include <cstdlib>

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

	m_strToken = ini::CINIHandler::InstanceRef().GetValue(ini::Config::System, "HTrader", "token", std::string());
	if (m_strToken.empty())
	{
		m_nErrorCode = 2;
		m_strLastError = "HQMarket token is required in ini/system.ini";
		net::EnvCleanup();
		return false;
	}

	std::string strHQMarketServer = ini::CINIHandler::InstanceRef().GetValue(ini::Config::System, "System", "hqmarket_server", std::string());
	std::string strHQMarketPort = ini::CINIHandler::InstanceRef().GetValue(ini::Config::System, "System", "hqmarket_port", std::string());
	if (strHQMarketServer.empty() || strHQMarketPort.empty())
	{
		m_nErrorCode = 3;
		m_strLastError = "HQMarket hqmarket_server && hqmarket_port is required in ini/system.ini";
		net::EnvCleanup();
		return false;
	}

	int nHQMarketPort = std::atoi(strHQMarketPort.c_str());
	m_pTcpClient = std::make_unique<net::CTcpClient>(strHQMarketServer, nHQMarketPort);

	m_bInitialized = true;
	return true;
}

bool CBootLoader::Run()
{
	if (!m_bInitialized || (nullptr == m_pTcpClient))
	{
		m_nErrorCode = 4;
		m_strLastError = "Boot loader is not initialized";
		return false;
	}
	if (0 != m_pTcpClient->Initialize())
	{
		m_nErrorCode = 4;
		m_strLastError = "HQMarket TCP client initialization failed";
		return false;
	}

	m_pTcpClient->Start(true);
	return true;
}

void CBootLoader::Stop()
{
	if (nullptr != m_pTcpClient)
	{
		m_pTcpClient->ShutDown();
	}
}

void CBootLoader::Finalize()
{
	if (!m_bInitialized)
	{
		return;
	}

	Stop();
	m_pTcpClient.reset();
	m_bInitialized = false;
	net::EnvCleanup();
}

net::CTcpClient& CBootLoader::GetTcpClient()
{
	return *m_pTcpClient;
}

const std::string& CBootLoader::GetToken() const
{
	return m_strToken;
}

const std::string& CBootLoader::GetLastError() const
{
	return m_strLastError;
}

int CBootLoader::GetErrorCode() const
{
	return m_nErrorCode;
}
