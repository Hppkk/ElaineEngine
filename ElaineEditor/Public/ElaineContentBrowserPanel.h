#pragma once
#include "ElaineEditorBase.h"
#include "Resource/ElaineResourceRegistry.h"
#include "ElaineResourceImportService.h"
namespace Editor
{
    class ContentBrowserPanel : public EditorPanel
    {
    public:
        ContentBrowserPanel();
        void OnDraw() override;
        void BeginImport(bool Texture);
    private:
        void DrawFolders(const std::filesystem::path& Path);
        void DrawAssetTooltip(const Elaine::ResourceDescriptor& Record);
        void DrawAssetContextMenu(const Elaine::ResourceDescriptor* Record);
        void DrawOperations();
        void Navigate(const std::string& Path);
        std::filesystem::path mRoot;
        std::string mDirectory, mSelected, mError, mImportReport;
        char mSearch[256]{}, mTag[128]{}, mValue[128]{}, mName[256]{};
        int mType = 0, mDimension = 0;
        bool mShowSources = false, mRename = false, mDelete = false, mImport = false, mSRGB = true;
        std::vector<std::filesystem::path> mSources;
        bool mTextureImport = true;
        float mMeshScale = 1.0f;
        std::shared_ptr<Elaine::ResourceImportResult> mImportResult;
        TaskGraph::TaskCompletionEventPtr mPending;
    };
}
