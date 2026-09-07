#include "CBootLoader.h"
#include "../network/common_net.h"

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
