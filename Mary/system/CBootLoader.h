#ifndef HQMARKET_SYSTEM_CBOOTLOADER_H
#define HQMARKET_SYSTEM_CBOOTLOADER_H

#include <memory>
#include <string>
#include <filesystem>

namespace net
{
	class CTcpClient;
}

class CBootLoader final
{
	public:
		CBootLoader();
		~CBootLoader();
		CBootLoader(const CBootLoader&) = delete;
		CBootLoader& operator=(const CBootLoader&) = delete;

		bool Initialize();
		bool Run();
		void Stop();
		void Finalize();
		const std::filesystem::path& GetRoot() const;
		net::CTcpClient& GetTcpClient();
		const std::string& GetToken() const;
		const std::string& GetLastError() const;
		int GetErrorCode() const;

	private:
		std::filesystem::path m_exec;
		std::filesystem::path m_path_py_runtime;
		std::filesystem::path m_path_py_scripts;
		std::string m_strToken;
		std::string m_strLastError;
		int m_nErrorCode{ 0 };
		bool m_bInitialized{ false };

	private:
		std::unique_ptr<net::CTcpClient> m_pTcpClient;
};

#endif
