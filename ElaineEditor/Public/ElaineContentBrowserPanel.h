#pragma once
#include "ElaineEditorBase.h"
#include <string>
#include <vector>
#include <filesystem>

namespace Editor
{
	// ============================================================
	// ContentBrowserPanel — Windows-Explorer-style file browser
	// rooted at the engine's Contents/ directory
	// ============================================================
	class ContentBrowserPanel : public EditorPanel
	{
	public:
		ContentBrowserPanel();
		void OnDraw() override;

	private:
		// Directory entry for display
		struct DirEntry
		{
			std::string Name;
			std::string FullPath;
			bool        IsDirectory = false;
			uintmax_t   FileSize = 0;
			std::string Extension;
		};

		// Navigation
		void NavigateTo(const std::filesystem::path& dir);
		void NavigateUp();
		void RefreshEntries();

		// Drawing sub-sections
		void DrawToolbar();
		void DrawFolderTree(const std::filesystem::path& root, const std::filesystem::path& currentRelative);
		void DrawContentArea();
		void DrawBreadcrumb();
		void DrawContextMenu();
		void DrawRenamePopup();

		// Actions
		void ImportMesh();
		void CreateNewFolder();
		void DeleteSelected();
		void RenameSelected();
		void OpenInExplorer(const std::string& path);
		void CopyPathToClipboard(const std::string& path);

		// State
		std::filesystem::path   mRootDir;          // Contents/ absolute path
		std::filesystem::path   mCurrentDir;       // Current browsing directory
		std::vector<DirEntry>   mEntries;          // Cached directory entries
		int                     mSelectedIndex = -1;
		char                    mSearchFilter[256] = {};

		// Rename state
		bool                    mShowRenamePopup = false;
		char                    mRenameBuffer[256] = {};
		int                     mRenameIndex = -1;

		// History for back/forward
		std::vector<std::filesystem::path> mHistory;
		int                     mHistoryIndex = -1;
	};
}
