#include "ElaineContentBrowserPanel.h"
#include "ElaineRoot.h"
#include "ElaineMesh.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <shlobj.h>
#include <commdlg.h>
#include <shellapi.h>
#include <Windows.h>

namespace fs = std::filesystem;

namespace Editor
{
	ContentBrowserPanel::ContentBrowserPanel()
		: EditorPanel("Content Browser")
	{
		// Root = engine's Contents/ directory
		mRootDir = fs::path(Elaine::Root::instance()->GetResourcePath());
		if (!fs::exists(mRootDir))
			mRootDir = fs::current_path();

		mRootDir = fs::canonical(mRootDir);
		NavigateTo(mRootDir);
	}

	void ContentBrowserPanel::NavigateTo(const fs::path& dir)
	{
		if (!fs::exists(dir) || !fs::is_directory(dir))
			return;

		mCurrentDir = fs::canonical(dir);
		mSelectedIndex = -1;

		// History management
		if (mHistoryIndex < 0 || mHistory[mHistoryIndex] != mCurrentDir)
		{
			// Trim forward history
			if (mHistoryIndex + 1 < (int)mHistory.size())
				mHistory.erase(mHistory.begin() + mHistoryIndex + 1, mHistory.end());

			mHistory.push_back(mCurrentDir);
			mHistoryIndex = (int)mHistory.size() - 1;
		}

		RefreshEntries();
	}

	void ContentBrowserPanel::NavigateUp()
	{
		if (mCurrentDir.has_parent_path() && mCurrentDir.parent_path() != mCurrentDir)
		{
			// Don't go above root
			fs::path parent = mCurrentDir.parent_path();
			// Allow going above root for flexibility
			NavigateTo(parent);
		}
	}

	void ContentBrowserPanel::RefreshEntries()
	{
		mEntries.clear();
		std::error_code ec;
		for (auto& entry : fs::directory_iterator(mCurrentDir, ec))
		{
			DirEntry de;
			de.Name = entry.path().filename().string();
			de.FullPath = entry.path().string();
			de.IsDirectory = entry.is_directory(ec);
			
			if (!de.IsDirectory)
			{
				de.FileSize = entry.file_size(ec);
				de.Extension = entry.path().extension().string();
			}

			mEntries.push_back(de);
		}

		// Sort: directories first, then alphabetical
		std::sort(mEntries.begin(), mEntries.end(), [](const DirEntry& a, const DirEntry& b)
		{
			if (a.IsDirectory != b.IsDirectory)
				return a.IsDirectory > b.IsDirectory;
			return a.Name < b.Name;
		});
	}

	// ============================================================
	// Main Draw
	// ============================================================
	void ContentBrowserPanel::OnDraw()
	{
		DrawToolbar();
		DrawBreadcrumb();

		ImGui::Separator();

		// Two-column layout: folder tree | content area
		float treeWidth = 180.0f;
		
		ImGui::BeginChild("FolderTreePane", ImVec2(treeWidth, 0), true);
		DrawFolderTree(mRootDir, mRootDir);
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("ContentAreaPane", ImVec2(0, 0), true);
		DrawContentArea();
		ImGui::EndChild();

		// Context menu & rename popup
		DrawRenamePopup();
	}

	// ============================================================
	// Toolbar: Back / Forward / Up / Refresh / Search
	// ============================================================
	void ContentBrowserPanel::DrawToolbar()
	{
		// Back button
		bool canBack = mHistoryIndex > 0;
		if (!canBack) ImGui::BeginDisabled();
		if (ImGui::Button("<"))
		{
			mHistoryIndex--;
			mCurrentDir = mHistory[mHistoryIndex];
			mSelectedIndex = -1;
			RefreshEntries();
		}
		if (!canBack) ImGui::EndDisabled();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back");

		ImGui::SameLine();

		// Forward button
		bool canForward = mHistoryIndex < (int)mHistory.size() - 1;
		if (!canForward) ImGui::BeginDisabled();
		if (ImGui::Button(">"))
		{
			mHistoryIndex++;
			mCurrentDir = mHistory[mHistoryIndex];
			mSelectedIndex = -1;
			RefreshEntries();
		}
		if (!canForward) ImGui::EndDisabled();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Forward");

		ImGui::SameLine();

		// Up button
		bool canUp = mCurrentDir != mRootDir && mCurrentDir.has_parent_path();
		if (!canUp) ImGui::BeginDisabled();
		if (ImGui::Button("^"))
		{
			NavigateUp();
		}
		if (!canUp) ImGui::EndDisabled();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Up");

		ImGui::SameLine();

		// Refresh
		if (ImGui::Button("Refresh"))
		{
			RefreshEntries();
		}

		ImGui::SameLine();

		// Search filter
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputTextWithHint("##search", "Search...", mSearchFilter, sizeof(mSearchFilter));
	}

	// ============================================================
	// Breadcrumb navigation
	// ============================================================
	void ContentBrowserPanel::DrawBreadcrumb()
	{
		// Build path segments relative to root
		fs::path relPath = fs::relative(mCurrentDir, mRootDir.parent_path());
		
		fs::path accumulated = mRootDir.parent_path();
		bool first = true;

		for (auto& segment : relPath)
		{
			if (!first)
			{
				ImGui::SameLine();
				ImGui::TextUnformatted(">");
				ImGui::SameLine();
			}
			first = false;

			accumulated /= segment;
			std::string label = segment.string();

			ImGui::PushID(label.c_str());
			if (ImGui::SmallButton(label.c_str()))
			{
				if (fs::exists(accumulated) && fs::is_directory(accumulated))
					NavigateTo(accumulated);
			}
			ImGui::PopID();
		}
	}

	// ============================================================
	// Folder tree (left sidebar)
	// ============================================================
	void ContentBrowserPanel::DrawFolderTree(const fs::path& root, const fs::path& currentRelative)
	{
		std::error_code ec;
		for (auto& entry : fs::directory_iterator(root, ec))
		{
			if (!entry.is_directory(ec))
				continue;

			std::string name = entry.path().filename().string();
			bool isSelected = (entry.path() == mCurrentDir);

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (isSelected)
				flags |= ImGuiTreeNodeFlags_Selected;

			// Check if has sub-directories
			bool hasSubDirs = false;
			for (auto& sub : fs::directory_iterator(entry.path(), ec))
			{
				if (sub.is_directory(ec))
				{
					hasSubDirs = true;
					break;
				}
			}
			if (!hasSubDirs)
				flags |= ImGuiTreeNodeFlags_Leaf;

			bool opened = ImGui::TreeNodeEx(name.c_str(), flags);

			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			{
				NavigateTo(entry.path());
			}

			if (opened)
			{
				DrawFolderTree(entry.path(), currentRelative);
				ImGui::TreePop();
			}
		}
	}

	// ============================================================
	// Content area (right side — file/folder grid)
	// ============================================================
	void ContentBrowserPanel::DrawContentArea()
	{
		// Right-click on blank area
		if (ImGui::BeginPopupContextWindow("ContentAreaContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("New Folder"))
			{
				CreateNewFolder();
			}
			if (ImGui::MenuItem("Import Mesh..."))
			{
				ImportMesh();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Refresh"))
			{
				RefreshEntries();
			}
			if (ImGui::MenuItem("Open in Explorer"))
			{
				OpenInExplorer(mCurrentDir.string());
			}
			ImGui::EndPopup();
		}

		// Calculate grid layout
		float iconSize = 80.0f;
		float cellSize = iconSize + 16.0f;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columns = (int)(panelWidth / cellSize);
		if (columns < 1) columns = 1;

		// Filter entries by search
		std::string filterStr(mSearchFilter);
		std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

		int col = 0;
		for (int i = 0; i < (int)mEntries.size(); ++i)
		{
			auto& entry = mEntries[i];

			// Apply search filter
			if (!filterStr.empty())
			{
				std::string lowerName = entry.Name;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
				if (lowerName.find(filterStr) == std::string::npos)
					continue;
			}

			ImGui::PushID(i);

			// Start new row if needed
			if (col > 0)
				ImGui::SameLine();

			// Draw icon + label as a group
			ImGui::BeginGroup();
			{
				bool isSelected = (mSelectedIndex == i);

				// Icon button — use color to distinguish folders vs files
				if (entry.IsDirectory)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.24f, 0.10f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.35f, 0.15f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.43f, 0.18f, 1.0f));
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.20f, 0.35f, 0.55f, 1.0f) : ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.40f, 0.60f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.45f, 0.65f, 1.0f));
				}

				if (isSelected)
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
					ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
				}

				// Icon area
				ImGui::Button(entry.IsDirectory ? "D" : "F", ImVec2(iconSize, iconSize));

				if (isSelected)
				{
					ImGui::PopStyleColor(); // Border
					ImGui::PopStyleVar();   // FrameBorderSize
				}

				ImGui::PopStyleColor(3);

				// Single click = select
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					mSelectedIndex = i;
				}

				// Double click
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (entry.IsDirectory)
					{
						NavigateTo(entry.FullPath);
					}
				}

				// Right-click context menu on item
				if (ImGui::BeginPopupContextItem("ItemContextMenu"))
				{
					mSelectedIndex = i;

					if (entry.IsDirectory)
					{
						if (ImGui::MenuItem("Open"))
						{
							NavigateTo(entry.FullPath);
							ImGui::EndPopup();
							ImGui::EndGroup();
							ImGui::PopID();
							return; // NavigateTo invalidates entries
						}
						if (ImGui::MenuItem("Open in Explorer"))
						{
							OpenInExplorer(entry.FullPath);
						}
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Rename"))
					{
						RenameSelected();
					}
					if (ImGui::MenuItem("Delete"))
					{
						DeleteSelected();
						ImGui::EndPopup();
						ImGui::EndGroup();
						ImGui::PopID();
						return; // DeleteSelected invalidates entries
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Copy Path"))
					{
						CopyPathToClipboard(entry.FullPath);
					}

					ImGui::EndPopup();
				}

				// Label (truncated)
				float textWidth = iconSize;
				std::string displayName = entry.Name;
				ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
				if (textSize.x > textWidth)
				{
					// Truncate with ellipsis
					while (displayName.size() > 3 && ImGui::CalcTextSize(displayName.c_str()).x > textWidth)
						displayName.pop_back();
					if (displayName.size() >= 3)
					{
						displayName.pop_back();
						displayName.pop_back();
						displayName.pop_back();
						displayName += "...";
					}
				}
				ImGui::TextWrapped("%s", displayName.c_str());
			}
			ImGui::EndGroup();

			col++;
			if (col >= columns)
				col = 0;

			ImGui::PopID();
		}
	}

	// ============================================================
	// Context Menu Actions
	// ============================================================
	void ContentBrowserPanel::ImportMesh()
	{
		char szFile[MAX_PATH] = {};
		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "3D Models\0*.obj;*.fbx;*.gltf;*.glb;*.dae;*.ply;*.stl\0All Files\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn))
		{
			std::string srcPath = szFile;
			fs::path srcFs(srcPath);
			std::string stem = srcFs.stem().string();
			std::string destRelPath = "model/" + stem + ".emesh";

			// Ensure model directory exists
			fs::path modelDir = mRootDir / "model";
			if (!fs::exists(modelDir))
				fs::create_directories(modelDir);

#ifdef _HAS_EDITOR_
			Elaine::ImportMeshFromFile(srcPath, destRelPath);
#endif

			// Navigate to model dir and refresh
			NavigateTo(modelDir);
		}
	}

	void ContentBrowserPanel::CreateNewFolder()
	{
		std::string baseName = "NewFolder";
		fs::path newPath = mCurrentDir / baseName;
		int counter = 1;
		while (fs::exists(newPath))
		{
			newPath = mCurrentDir / (baseName + std::to_string(counter));
			counter++;
		}

		std::error_code ec;
		fs::create_directory(newPath, ec);
		RefreshEntries();
	}

	void ContentBrowserPanel::DeleteSelected()
	{
		if (mSelectedIndex < 0 || mSelectedIndex >= (int)mEntries.size())
			return;

		auto& entry = mEntries[mSelectedIndex];
		std::error_code ec;

		if (entry.IsDirectory)
			fs::remove_all(entry.FullPath, ec);
		else
			fs::remove(entry.FullPath, ec);

		mSelectedIndex = -1;
		RefreshEntries();
	}

	void ContentBrowserPanel::RenameSelected()
	{
		if (mSelectedIndex < 0 || mSelectedIndex >= (int)mEntries.size())
			return;

		mRenameIndex = mSelectedIndex;
		strncpy_s(mRenameBuffer, mEntries[mSelectedIndex].Name.c_str(), sizeof(mRenameBuffer) - 1);
		mShowRenamePopup = true;
		ImGui::OpenPopup("RenamePopup");
	}

	void ContentBrowserPanel::DrawRenamePopup()
	{
		if (!mShowRenamePopup)
			return;

		if (!ImGui::IsPopupOpen("RenamePopup"))
			ImGui::OpenPopup("RenamePopup");

		if (ImGui::BeginPopupModal("RenamePopup", &mShowRenamePopup, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("New name:");
			ImGui::InputText("##rename", mRenameBuffer, sizeof(mRenameBuffer));

			if (ImGui::Button("OK", ImVec2(100, 0)))
			{
				if (mRenameIndex >= 0 && mRenameIndex < (int)mEntries.size())
				{
					fs::path oldPath = mEntries[mRenameIndex].FullPath;
					fs::path newPath = oldPath.parent_path() / mRenameBuffer;
					std::error_code ec;
					fs::rename(oldPath, newPath, ec);
					RefreshEntries();
				}
				mShowRenamePopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(100, 0)))
			{
				mShowRenamePopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::OpenInExplorer(const std::string& path)
	{
		ShellExecuteA(NULL, "explore", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	void ContentBrowserPanel::CopyPathToClipboard(const std::string& path)
	{
		if (OpenClipboard(NULL))
		{
			EmptyClipboard();
			HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, path.size() + 1);
			if (hg)
			{
				memcpy(GlobalLock(hg), path.c_str(), path.size() + 1);
				GlobalUnlock(hg);
				SetClipboardData(CF_TEXT, hg);
			}
			CloseClipboard();
		}
	}
}
