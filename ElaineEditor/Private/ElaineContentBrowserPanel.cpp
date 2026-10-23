#include "ElaineContentBrowserPanel.h"
#include "ElaineRoot.h"
#include "ElaineMesh.h"
#include "Resource/ElaineResourceLoader.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "imgui.h"
#include <Windows.h>
#include <commdlg.h>
#include <shellapi.h>
namespace fs = std::filesystem;
using namespace Elaine;
namespace Editor
{
    namespace
    {
        ImU32 AssetColor(const std::string& Type)
        {
            if (Type == "Texture") return IM_COL32(60, 145, 180, 255);
            if (Type == "Mesh") return IM_COL32(80, 115, 195, 255);
            if (Type == "Material" || Type == "MaterialInstance") return IM_COL32(75, 145, 95, 255);
            if (Type == "Shader") return IM_COL32(185, 125, 60, 255);
            if (Type == "Actor") return IM_COL32(165, 115, 165, 255);
            if (Type == "World") return IM_COL32(65, 150, 145, 255);
            return IM_COL32(110, 115, 125, 255);
        }

        bool DrawAssetTile(const std::string& Label, const std::string& Type, bool Selected, bool HasError = false)
        {
            const ImVec2 Position = ImGui::GetCursorScreenPos();
            const float Width = ImGui::GetContentRegionAvail().x;
            const float LineHeight = ImGui::GetTextLineHeight();
            const float IconSize = (std::min)(72.0f, (std::max)(24.0f, Width - 16.0f));
            const float Height = IconSize + LineHeight * 3.0f + 20.0f;
            const bool Clicked = ImGui::Selectable("##AssetTile", Selected,
                ImGuiSelectableFlags_AllowDoubleClick, ImVec2(Width, Height));
            auto* DrawList = ImGui::GetWindowDrawList();
            const ImVec2 IconMin(Position.x + (Width - IconSize) * 0.5f, Position.y + 6.0f);
            const ImVec2 IconMax(IconMin.x + IconSize, IconMin.y + IconSize);
            DrawList->AddRectFilled(IconMin, IconMax, AssetColor(Type), 6.0f);
            DrawList->AddRect(ImVec2(IconMin.x + 14, IconMin.y + 10), ImVec2(IconMax.x - 14, IconMax.y - 10),
                IM_COL32(255, 255, 255, 150), 2.0f);
            const char* Glyph = Type == "Texture" ? "TEX" : Type == "Mesh" ? "MESH" :
                Type == "Material" ? "MAT" : Type == "MaterialInstance" ? "MI" :
                Type == "Shader" ? "SH" : Type == "Actor" ? "ACT" : Type == "World" ? "MAP" : "FILE";
            const auto TextSize = ImGui::CalcTextSize(Glyph);
            DrawList->AddText(ImVec2(IconMin.x + (IconSize - TextSize.x) * 0.5f, IconMin.y + (IconSize - TextSize.y) * 0.5f),
                IM_COL32(255, 255, 255, 255), Glyph);
            const ImVec2 TextMin(Position.x + 4, IconMax.y + 6);
            DrawList->PushClipRect(TextMin, ImVec2(Position.x + Width - 4, TextMin.y + LineHeight * 2), true);
            DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), TextMin, ImGui::GetColorU32(ImGuiCol_Text),
                Label.c_str(), nullptr, Width - 8);
            DrawList->PopClipRect();
            DrawList->AddText(ImVec2(Position.x + 4, TextMin.y + LineHeight * 2 + 3),
                ImGui::GetColorU32(ImGuiCol_TextDisabled), Type.c_str());
            if (HasError) DrawList->AddText(ImVec2(IconMax.x - 10, IconMin.y + 2), IM_COL32(255, 110, 100, 255), "!");
            return Clicked;
        }

        int AssetColumns()
        { return (std::max)(1, static_cast<int>(ImGui::GetContentRegionAvail().x / 128.0f)); }
    }

    ContentBrowserPanel::ContentBrowserPanel() : EditorPanel("Content Browser")
    { mRoot = fs::weakly_canonical(Root::instance()->GetResourcePath()); }
    void ContentBrowserPanel::Navigate(const std::string& Path)
    { mDirectory = Path; mSelected.clear(); }
    void ContentBrowserPanel::DrawFolders(const fs::path& Path)
    {
        auto Relative = Path.lexically_relative(mRoot).generic_string(); if (Relative == ".") Relative.clear();
        const auto Label = Relative.empty() ? "Contents" : Path.filename().string();
        auto Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (Relative == mDirectory) Flags |= ImGuiTreeNodeFlags_Selected;
        if (Relative.empty()) Flags |= ImGuiTreeNodeFlags_DefaultOpen;
        ImGui::PushID(Relative.c_str()); const bool Open = ImGui::TreeNodeEx(Label.c_str(), Flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) Navigate(Relative);
        if (Open)
        {
            std::error_code Error;
            for (const auto& Entry : fs::directory_iterator(Path, Error))
                if (Entry.is_directory(Error) && !Entry.is_symlink(Error)) DrawFolders(Entry.path());
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    void ContentBrowserPanel::OnDraw()
    {
        if (mPending && mPending->IsComplete())
        {
            try { mPending->RethrowIfFailed(); if (mPending->GetState() == TaskGraph::TaskState::Cancelled) mError = "Operation cancelled"; }
            catch (const std::exception& Error) { mError = Error.what(); }
            mPending.reset();
        }
        if (mImportResult && (mImportResult->Primary.Id.IsValid() || !mImportResult->Error.empty()))
        {
            mImportReport = mImportResult->Succeeded() ? "Import completed" : "Import failed";
            for (const auto& Asset : mImportResult->Assets)
                mImportReport += "\n" + Asset.Type + ": " + Asset.Path;
            for (const auto& Asset : mImportResult->UnusedAssets)
                mImportReport += "\nUnused (retained): " + Asset.Path;
            if (!mImportResult->Error.empty()) mError = mImportResult->Error;
            for (const auto& Warning : mImportResult->Warnings) { if (!mError.empty()) mError += "\n"; mError += Warning; }
            mImportResult.reset();
        }
        if (ImGui::Button("Up") && !mDirectory.empty()) Navigate(fs::path(mDirectory).parent_path().generic_string());
        ImGui::SameLine(); if (ImGui::Button("All assets")) Navigate("");
        ImGui::SameLine(); ImGui::TextUnformatted(("Contents/" + mDirectory).c_str());
        ImGui::SetNextItemWidth(180); ImGui::InputText("Search", mSearch, sizeof(mSearch));
        ImGui::SameLine(); ImGui::SetNextItemWidth(130);
        ImGui::Combo("Type", &mType, "All\0Texture\0Mesh\0Material\0MaterialInstance\0Shader\0Actor\0World\0");
        ImGui::SetNextItemWidth(150); ImGui::InputText("Property", mTag, sizeof(mTag));
        ImGui::SameLine(); ImGui::SetNextItemWidth(150); ImGui::InputText("Value", mValue, sizeof(mValue));
        ImGui::SameLine(); ImGui::Checkbox("Source files", &mShowSources);
        if (mPending) ImGui::TextUnformatted("Operation in progress...");
        if (!mError.empty()) { ImGui::TextWrapped("%s", mError.c_str()); if (ImGui::SmallButton("Dismiss")) mError.clear(); }
        if (!mImportReport.empty())
        {
            if (ImGui::CollapsingHeader("Import results")) ImGui::TextWrapped("%s", mImportReport.c_str());
            if (ImGui::SmallButton("Clear import results")) mImportReport.clear();
        }
        ImGui::Separator();
        ImGui::BeginChild("Directories", ImVec2(170, 0), true); DrawFolders(mRoot); ImGui::EndChild(); ImGui::SameLine();
        ImGui::BeginChild("Assets", ImVec2(0, 0), true);
        ResourceQuery Query; Query.Directory = mDirectory; Query.Search = mSearch; Query.Tag = mTag; Query.Value = mValue;
        static const char* Types[] = {"", "Texture", "Mesh", "Material", "MaterialInstance", "Shader", "Actor", "World"};
        Query.Type = Types[mType];
        const auto Records = ResourceRegistry::Get().Query(Query);
        if (ImGui::BeginTable("AssetTiles", AssetColumns(), ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        {
            for (const auto& Record : Records)
            {
                ImGui::TableNextColumn(); ImGui::PushID(Record.Path.c_str());
                const auto Label = Record.DisplayName.empty() ? fs::path(Record.Path).stem().string() : Record.DisplayName;
                if (DrawAssetTile(Label, Record.Type, mSelected == Record.Path, !Record.Error.empty())) mSelected = Record.Path;
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
                    !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) DrawAssetTooltip(Record);
                if (ImGui::BeginPopupContextItem("AssetActions"))
                {
                    mSelected = Record.Path; DrawAssetContextMenu(&Record); ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        if (Records.empty()) ImGui::TextDisabled("No matching assets");
        if (mShowSources)
        {
            ImGui::SeparatorText("Source files");
            if (ImGui::BeginTable("SourceTiles", AssetColumns(), ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
            {
                std::error_code Error;
                for (const auto& Entry : fs::directory_iterator(mRoot / mDirectory, Error))
                    if (Entry.is_regular_file() && Entry.path().extension() != ".asset" && Entry.path().extension() != ".shader")
                    {
                        ImGui::TableNextColumn(); const auto Path = Entry.path().generic_string(); ImGui::PushID(Path.c_str());
                        if (DrawAssetTile(Entry.path().filename().string(), "Source", false) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            ShellExecuteW(nullptr, L"open", Entry.path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s\nDouble-click to open", Path.c_str());
                        ImGui::PopID();
                    }
                ImGui::EndTable();
            }
        }
        if (ImGui::BeginPopupContextWindow("AssetAreaActions", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        { DrawAssetContextMenu(nullptr); ImGui::EndPopup(); }
        ImGui::EndChild();
        DrawOperations();
    }
    void ContentBrowserPanel::DrawAssetTooltip(const ResourceDescriptor& Record)
    {
        ImGui::BeginTooltip(); ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);
        ImGui::TextUnformatted((Record.DisplayName.empty() ? fs::path(Record.Path).stem().string() : Record.DisplayName).c_str());
        ImGui::TextDisabled("%s", Record.Type.c_str()); ImGui::Separator();
        ImGui::TextWrapped("Path: %s", Record.Path.c_str());
        ImGui::TextWrapped("ID: %s", Record.Id.ToString().c_str());
        if (!Record.Error.empty()) ImGui::TextWrapped("Error: %s", Record.Error.c_str());
        if (!Record.Tags.empty()) { ImGui::SeparatorText("Properties"); ImGui::TextWrapped("%s", Record.Tags.dump(2).c_str()); }
        if (!Record.Dependencies.empty())
        {
            ImGui::SeparatorText("Dependencies");
            for (const auto& Dependency : Record.Dependencies)
                ImGui::TextWrapped("%s: %s", Dependency.Kind == ResourceDependencyKind::Hard ? "Hard" : "Soft", Dependency.Reference.Path.c_str());
        }
        const auto Referencers = ResourceRegistry::Get().GetReferencers(Record.Id);
        if (!Referencers.empty())
        {
            ImGui::SeparatorText("Referenced by");
            for (const auto& Target : Referencers) ImGui::TextWrapped("%s", Target.Path.c_str());
        }
        if (!Record.SourceFiles.empty())
        {
            ImGui::SeparatorText("Shader sources / includes");
            for (const auto& Source : Record.SourceFiles) ImGui::TextWrapped("%s", Source.c_str());
        }
        ImGui::PopTextWrapPos(); ImGui::EndTooltip();
    }
    void ContentBrowserPanel::DrawAssetContextMenu(const ResourceDescriptor* Record)
    {
        ImGui::BeginDisabled(static_cast<bool>(mPending));
        if (ImGui::MenuItem("Import texture...")) BeginImport(true);
        if (ImGui::MenuItem("Import mesh...")) BeginImport(false);
        ImGui::EndDisabled();
        if (!Record) return;
        ImGui::Separator();
        if (ImGui::MenuItem("Copy ID")) ImGui::SetClipboardText(Record->Id.ToString().c_str());
        if (ImGui::MenuItem("Copy path")) ImGui::SetClipboardText(Record->Path.c_str());
        ImGui::BeginDisabled(static_cast<bool>(mPending));
        if (ImGui::MenuItem("Rename / move...")) { strcpy_s(mName, Record->Path.c_str()); mRename = true; }
        if ((Record->Type == "Texture" || Record->Type == "Mesh") && ImGui::MenuItem("Reimport asset"))
        {
            try { mImportResult = std::make_shared<ResourceImportResult>();
                mPending = ResourceImportService::Get().Reimport(Record->Path, [Result=mImportResult](const auto& Value) { *Result = Value; }); }
            catch (const std::exception& Error) { mError = Error.what(); }
        }
        if (ImGui::MenuItem("Delete...")) mDelete = true;
        ImGui::EndDisabled();
        if (!Record->Dependencies.empty() && ImGui::BeginMenu("Dependencies"))
        {
            for (const auto& Dependency : Record->Dependencies)
            {
                const auto& Reference = Dependency.Reference; ImGui::PushID(Reference.Id.ToString().c_str());
                try { const auto Target = ResourceRegistry::Get().Find(Reference.Id);
                    if (ImGui::MenuItem(Target.Path.c_str())) { Navigate(fs::path(Target.Path).parent_path().generic_string()); mSelected = Target.Path; } }
                catch (const std::exception&) { ImGui::TextDisabled("Missing: %s", Reference.Path.c_str()); }
                ImGui::PopID();
            }
            ImGui::EndMenu();
        }
        const auto Referencers = ResourceRegistry::Get().GetReferencers(Record->Id);
        if (!Referencers.empty() && ImGui::BeginMenu("Referenced by"))
        {
            for (const auto& Target : Referencers)
                if (ImGui::MenuItem(Target.Path.c_str())) { Navigate(fs::path(Target.Path).parent_path().generic_string()); mSelected = Target.Path; }
            ImGui::EndMenu();
        }
    }
    void ContentBrowserPanel::BeginImport(bool Texture)
    {
        char Files[32768]{}; OPENFILENAMEA Dialog{}; Dialog.lStructSize = sizeof(Dialog); Dialog.lpstrFile = Files;
        std::string Patterns;
        for (const auto& Extension : ResourceImportService::Get().GetSupportedExtensions(Texture ? "Texture" : "Mesh"))
        { if (!Patterns.empty()) Patterns += ";"; Patterns += "*" + Extension; }
        std::string Filter = Texture ? "Images" : "Meshes"; Filter.push_back('\0'); Filter += Patterns; Filter.push_back('\0'); Filter += "All"; Filter.push_back('\0'); Filter += "*.*"; Filter.append(2,'\0');
        Dialog.nMaxFile = sizeof(Files); Dialog.lpstrFilter = Filter.c_str();
        Dialog.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR | (Texture ? OFN_ALLOWMULTISELECT : 0);
        if (!GetOpenFileNameA(&Dialog)) return;
        mSources.clear(); const char* Next = Files + strlen(Files) + 1;
        if (!*Next) mSources.emplace_back(Files);
        else while (*Next) { mSources.emplace_back(fs::path(Files) / Next); Next += strlen(Next) + 1; }
        strcpy_s(mName, (mSources.front().stem().string() + ".asset").c_str());
        mTextureImport = Texture;
        mDimension = mSources.size() == 6 ? 1 : (mSources.size() > 1 ? 2 : 0);
        mMeshScale = 1.0f; mImport = true;
    }

    void ContentBrowserPanel::DrawOperations()
    {
        if (mRename) { ImGui::OpenPopup("Move asset"); mRename = false; }
        if (ImGui::BeginPopupModal("Move asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Contents relative path", mName, sizeof(mName));
            if (ImGui::Button("Move")) { try { mPending = ResourceRegistry::Get().Move(mSelected, mName); mSelected = mName; ImGui::CloseCurrentPopup(); } catch (const std::exception& Error) { mError = Error.what(); } }
            ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
        }
        if (mDelete) { ImGui::OpenPopup("Delete asset"); mDelete = false; }
        if (ImGui::BeginPopupModal("Delete asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("Delete %s? References will retain the missing ID.", mSelected.c_str());
            try { for (const auto& Item : ResourceRegistry::Get().GetReferencers(ResourceRegistry::Get().Reference(mSelected).Id)) ImGui::TextUnformatted(Item.Path.c_str()); } catch (...) {}
            if (ImGui::Button("Confirm delete")) { try { mPending = ResourceRegistry::Get().Delete(mSelected, true); ImGui::CloseCurrentPopup(); } catch (const std::exception& Error) { mError = Error.what(); } }
            ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
        }
        if (mImport) { ImGui::OpenPopup("Import resource asset"); mImport = false; }
        if (ImGui::BeginPopupModal("Import resource asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Asset filename", mName, sizeof(mName));
            if (mTextureImport)
            { ImGui::Combo("Dimension", &mDimension, "2D\0Cubemap\0Array\0"); ImGui::Checkbox("sRGB (RGBA8 only)", &mSRGB); ImGui::TextUnformatted("Cubemap order: +X, -X, +Y, -Y, +Z, -Z"); }
            else { ImGui::InputFloat("Model scale", &mMeshScale); ImGui::TextUnformatted("Import geometry, PBR material instances and textures"); }
            for (size_t I = 0; I < mSources.size(); ++I)
            {
                ImGui::PushID(static_cast<int>(I));
                if (I && ImGui::SmallButton("Up")) std::swap(mSources[I], mSources[I - 1]);
                ImGui::SameLine(); ImGui::TextUnformatted(mSources[I].filename().string().c_str()); ImGui::PopID();
            }
            if (ImGui::Button("Import"))
            {
                try
                {
                    const auto Path = CanonicalResourcePath((fs::path(mDirectory) / mName).generic_string());
                    if (fs::path(mName).filename() != mName || fs::path(Path).extension() != ".asset") throw std::runtime_error("Enter a filename ending in .asset");
                    if (fs::exists(mRoot / Path)) throw std::runtime_error("Import destination already exists");
                    TextureImportSettings Settings; Settings.Dimension = mDimension == 0 ? "2d" : (mDimension == 1 ? "cube" : "2darray"); Settings.SRGB = mSRGB;
                    mImportResult = std::make_shared<ResourceImportResult>();
                    auto Completion = [Result=mImportResult](const ResourceImportResult& Value) { *Result = Value; };
                    if (mTextureImport) mPending = ResourceImportService::Get().ImportTexture(mSources, Path, Settings, Completion);
                    else { MeshImportSettings MeshSettings; MeshSettings.Scale = mMeshScale; mPending = ResourceImportService::Get().ImportMesh(mSources.front(), Path, MeshSettings, Completion); }
                    ImGui::CloseCurrentPopup();
                }
                catch (const std::exception& Error) { mError = Error.what(); }
            }
            ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
        }
    }
}
