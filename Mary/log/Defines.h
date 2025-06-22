#ifndef __LOGGER_DEFINES_H__
#define __LOGGER_DEFINES_H__
#include "CLogger.h"

#define LOGGER CLogger::InstancePtr()
#define MLOGGER(md) LOGGER->GetLogger(md)

#define MLOG_TRACE(md, ...) SPDLOG_LOGGER_CALL(LOGGER->GetLogger(md).get(), spdlog::level::trace, __VA_ARGS__)
#define MLOG_DEBUG(md, ...) SPDLOG_LOGGER_CALL(LOGGER->GetLogger(md).get(), spdlog::level::debug, __VA_ARGS__)
#define MLOG_INFO(md, ...) SPDLOG_LOGGER_CALL(LOGGER->GetLogger(md).get(), spdlog::level::info, __VA_ARGS__)
#define MLOG_WARN(md, ...) SPDLOG_LOGGER_CALL(LOGGER->GetLogger(md).get(), spdlog::level::warn, __VA_ARGS__)
#define MLOG_ERR(md, ...) SPDLOG_LOGGER_CALL(LOGGER->GetLogger(md).get(), spdlog::level::err, __VA_ARGS__)
#define MLOG_TRIVAL(md, ...) SPDLOG_LOGGER_CALL(LOGGER->GetLogger(md).get(), spdlog::level::critical, __VA_ARGS__)



#define GLOBAL_LOG_TRACE(...) MLOG_TRACE(md::modules::global, __VA_ARGS__)
#define GLOBAL_LOG_DEBUG(...) MLOG_DEBUG(md::modules::global, __VA_ARGS__)
#define GLOBAL_LOG_INFO(...) MLOG_INFO((md::modules::global, __VA_ARGS__)
#define GLOBAL_LOG_WARN(...) MLOG_WARN((md::modules::global, __VA_ARGS__)
#define GLOBAL_LOG_ERR(...) MLOG_ERR(md::modules::global, __VA_ARGS__)
#define GLOBAL_LOG_TRIVAL(...) MLOG_TRIVAL(md::modules::global, __VA_ARGS__)

#endif
