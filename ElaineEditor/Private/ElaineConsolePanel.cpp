#include "ElaineConsolePanel.h"
#include "imgui.h"
#include <cstring>
#include <algorithm>

namespace Editor
{
	void ConsolePanel::AddLog(const char* level, const char* message)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mLogs.push_back({ level, message });
		if (mLogs.size() > MAX_LOGS)
			mLogs.pop_front();
	}

	void ConsolePanel::Clear()
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mLogs.clear();
	}

	void ConsolePanel::OnDraw()
	{
		// Toolbar
		if (ImGui::Button("Clear"))
			Clear();

		ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &mAutoScroll);
		
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputText("Filter", mFilter, sizeof(mFilter));

		ImGui::Separator();

		// Log list
		ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false,
			ImGuiWindowFlags_HorizontalScrollbar);

		std::lock_guard<std::mutex> lock(mMutex);
		for (auto& entry : mLogs)
		{
			// Filter
			if (mFilter[0] != '\0')
			{
				if (entry.Message.find(mFilter) == std::string::npos &&
					entry.Level.find(mFilter) == std::string::npos)
					continue;
			}

			// Color by level
			ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
			if (entry.Level == "error" || entry.Level == "fatal")
				color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
			else if (entry.Level == "warning" || entry.Level == "warn")
				color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
			else if (entry.Level == "info")
				color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
			else if (entry.Level == "debug")
				color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextWrapped("[%s] %s", entry.Level.c_str(), entry.Message.c_str());
			ImGui::PopStyleColor();
		}

		if (mAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
	}
}
