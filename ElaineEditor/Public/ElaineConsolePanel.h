#pragma once
#include "ElaineEditorBase.h"
#include <string>
#include <vector>
#include <deque>
#include <mutex>

namespace Editor
{
	// ============================================================
	// ConsolePanel — shows engine log output
	// ============================================================
	class ConsolePanel : public EditorPanel
	{
	public:
		ConsolePanel()
			: EditorPanel("Console") {}

		void OnDraw() override;

		// Thread-safe: push log messages
		void AddLog(const char* level, const char* message);
		void Clear();

	private:
		struct LogEntry
		{
			std::string Level;
			std::string Message;
		};

		std::deque<LogEntry>  mLogs;
		std::mutex            mMutex;
		bool                  mAutoScroll = true;
		char                  mFilter[128] = {};
		static const size_t   MAX_LOGS = 1000;
	};
}
