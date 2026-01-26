#include "ElainePrecompiledHeader.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Elaine
{
	LogSystem::LogSystem()
	{

        spdlog::set_level(spdlog::level::trace);
        spdlog::set_pattern("%^[%Y-%m-%d %H:%M:%S][%l]:%v%$");
        m_logger = spdlog::stdout_color_mt("Elaine");
        m_logger->flush_on(spdlog::level::err);

        m_logFile.open(Root::instance()->GetAppPath() + "/ElaineLog.txt");
	}

	LogSystem::~LogSystem()
	{
        m_logger->flush();
        spdlog::drop_all();
        m_logFile.close();
	}

}