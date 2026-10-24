#include "CLogger.h"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <string>
#include <vector>
#include "../common/datetime.h"
#include <iostream>
CLogger::CLogger()
{
	Initialize();
}

CLogger::~CLogger()
{
	ShutDown();
}

//TRACE---DEBUG-----INFO----WARN----ERROR---CRITICAL
void CLogger::Initialize()
{
	spdlog::set_level(m_level);
	spdlog::flush_on(m_flush_level);
	spdlog::flush_every(std::chrono::seconds(5));
	spdlog::init_thread_pool(200, 1);
}

std::shared_ptr<spdlog::logger> CLogger::GetLogger(const std::string& strName)
{
	std::shared_ptr<spdlog::logger> logger = spdlog::get(strName);
	if (nullptr != logger)
	{
		return logger;
	}

	std::string strFileName = strName + "_" + datetime::GetSysDateTime(datetime::precision::year_day);
	std::string strFileFullName = GetRootDir() + strFileName + ".log";
	try
	{
		//每个文件大小限制50M  最多保留10个
		auto sinker = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(strFileFullName, 50 * 1024 * 1024, 10);
		sinker->set_level(m_level);

		std::shared_ptr<spdlog::async_logger> logger = std::make_shared<spdlog::async_logger>(
			strName,
			sinker,
			spdlog::thread_pool(),
			spdlog::async_overflow_policy::block);

		//输出格式 [2025-06-08 14:30:45.123] [info] Hello, world!
		logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
		logger->set_level(m_level);
		logger->flush_on(m_flush_level);
		if (nullptr == spdlog::get(strName))
		{
			spdlog::register_logger(logger);
			return logger;
		}
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Log initialization failed: " << ex.what() << std::endl;
	}

	return spdlog::get(strName);
}

std::string CLogger::GetRootDir() const
{
	return m_strDirRoot;
}

void CLogger::ShutDown()
{
	spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> CLogger::GetLogger(md::modules m)
{
	std::string strName = GetModuleName(m);
	if (strName.empty())
	{
		return nullptr;
	}
	return GetLogger(strName);
}