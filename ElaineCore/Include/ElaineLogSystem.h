#pragma once
#include "ElaineSingleton.h"
#include "spdlog/spdlog.h"

namespace Elaine
{
#ifndef LOG_HELPER
#define LOG_HELPER(LOG_LEVEL, ...) \
	LogSystem::instance()->log(LOG_LEVEL, "[" + std::string(__FUNCTION__) + "] " + __VA_ARGS__);
#endif // !LOG_HELPER
#ifndef LOG_DEBUG
#define LOG_DEBUG(...) LOG_HELPER(LogSystem::LogLevel::debug, __VA_ARGS__);
#endif
#ifndef LOG_INFO
#define LOG_INFO(...) LOG_HELPER(LogSystem::LogLevel::info, __VA_ARGS__);
#endif
#ifndef LOG_WARN
#define LOG_WARN(...) LOG_HELPER(LogSystem::LogLevel::warn, __VA_ARGS__);
#endif // !LOG_WARN
#ifndef LOG_ERROR
#define LOG_ERROR(...) LOG_HELPER(LogSystem::LogLevel::error, __VA_ARGS__);
#endif // !LOG_ERROR
#ifndef LOG_FATAL
#define LOG_FATAL(...) LOG_HELPER(LogSystem::LogLevel::fatal, __VA_ARGS__);
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
		void log(LogLevel level, TARGS&&... args)
		{
			std::string s;
			switch (level)
			{
			case LogLevel::debug:
				m_logger->debug(std::forward<TARGS>(args)...);
				s = fmt::format("", std::forward<TARGS>(args)...);
				m_logFile << s.c_str() << std::endl;
				break;
			case LogLevel::info:
				m_logger->info(std::forward<TARGS>(args)...);
				s = fmt::format("", std::forward<TARGS>(args)...);
				m_logFile << s.c_str() << std::endl;
				break;
			case LogLevel::warn:
				m_logger->warn(std::forward<TARGS>(args)...);
				s = fmt::format("", std::forward<TARGS>(args)...);
				m_logFile << s.c_str() << std::endl;
				break;
			case LogLevel::error:
				m_logger->error(std::forward<TARGS>(args)...);
				s = fmt::format("", std::forward<TARGS>(args)...);
				m_logFile << s.c_str() << std::endl;
				break;
			case LogLevel::fatal:
				m_logger->critical(std::forward<TARGS>(args)...);
				s = fmt::format("", std::forward<TARGS>(args)...);
				m_logFile << s.c_str() << std::endl;
				FatalCallFunction(std::forward<TARGS>(args)...);
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
	};
}