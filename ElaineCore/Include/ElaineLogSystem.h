#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "spdlog/spdlog.h"

namespace Elaine
{
#ifndef LOG_DEBUG
#define LOG_DEBUG(fmt, ...) \
	Elaine::LogSystem::instance()->log( \
		Elaine::LogSystem::LogLevel::debug, \
        " " fmt, \
        ##__VA_ARGS__ \
    );
#endif
#ifndef LOG_INFO
#define LOG_INFO(fmt, ...) \
    Elaine::LogSystem::instance()->log( \
        Elaine::LogSystem::LogLevel::info, \
        " " fmt, \
        ##__VA_ARGS__ \
    );
#endif
#ifndef LOG_WARN
#define LOG_WARN(fmt, ...) \
    Elaine::LogSystem::instance()->log( \
        Elaine::LogSystem::LogLevel::warn, \
        " " fmt, \
        ##__VA_ARGS__ \
    );
#endif // !LOG_WARN
#ifndef LOG_ERROR
#define LOG_ERROR(fmt, ...) \
    Elaine::LogSystem::instance()->log( \
        Elaine::LogSystem::LogLevel::error, \
        "[{}] " fmt, \
        __FUNCTION__, \
        ##__VA_ARGS__ \
    );
#endif // !LOG_ERROR
#ifndef LOG_FATAL
#define LOG_FATAL(fmt, ...) \
    Elaine::LogSystem::instance()->log( \
        Elaine::LogSystem::LogLevel::fatal, \
        "[{}] " fmt, \
        __FUNCTION__, \
        ##__VA_ARGS__ \
    );
#endif // !LOG_FATAL


	class ElaineCoreExport LogSystem : public Singleton<LogSystem>
	{
	public:
		enum LogLevel : uint8_t
		{
			debug,
			info,
			warn,
			error,
			fatal
		};
	public:
		LogSystem();
		~LogSystem();
		template<typename... TARGS>
		void log(LogLevel level, fmt::format_string<TARGS...> fmtStr, TARGS&&... args)
		{
			std::string msg = fmt::format(fmtStr, std::forward<TARGS>(args)...);
			std::lock_guard<std::mutex> Lock_Guard(mMtx);
			switch (level)
			{
			case LogLevel::debug:
				m_logger->debug(msg);
				m_logFile << msg.c_str() << std::endl;
				break;
			case LogLevel::info:
				m_logger->info(msg);
				m_logFile << msg.c_str() << std::endl;
				break;
			case LogLevel::warn:
				m_logger->warn(msg);
				m_logFile << msg.c_str() << std::endl;
				break;
			case LogLevel::error:
				m_logger->error(msg);
				m_logFile << msg.c_str() << std::endl;
				break;
			case LogLevel::fatal:
				m_logger->critical(msg);
				m_logFile << msg.c_str() << std::endl;
				FatalCallFunction(msg);
				break;
			default:
				break;
			}
		}

		template<typename... TARGS>
		void FatalCallFunction(TARGS&&... args)
		{
			const std::string format_str = std::format("{}", std::forward<TARGS>(args)...);
			throw std::runtime_error(format_str);
		}

	private:
		std::shared_ptr<spdlog::logger> m_logger;
		std::ofstream					m_logFile;
		std::mutex mMtx;
	};
}