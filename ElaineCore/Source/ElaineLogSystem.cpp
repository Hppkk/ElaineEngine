#include "ElainePrecompiledHeader.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Elaine
{
	LogSystem::LogSystem()
	{
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();


        sink->set_level(spdlog::level::trace);
        sink->set_pattern("[%^%l%$] %v");

        const spdlog::sinks_init_list sink_list = { sink };

        spdlog::init_thread_pool(8192, 1);

        m_logger = std::make_shared<spdlog::async_logger>("muggle_logger",
            sink_list.begin(),
            sink_list.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);
        m_logger->set_level(spdlog::level::trace);

        spdlog::register_logger(m_logger);
	}

	LogSystem::~LogSystem()
	{
        m_logger->flush();
        spdlog::drop_all();
	}

}