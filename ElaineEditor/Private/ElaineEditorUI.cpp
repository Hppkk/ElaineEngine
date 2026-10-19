#include "ElaineEditorUI.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Windows.h"
#include "ElaineMesh.h"
#include "ElaineRoot.h"
#include <commdlg.h>
#include <filesystem>

namespace Editor
{
	void EditorUI::Initialize(Elaine::ElaineEngine* engine)
	{
		mEngine = engine;
	}

	void EditorUI::AddPanel(EditorPanel* panel)
	{
		mPanels.push_back(panel);
	}

	// ============================================================
	// DrawDockSpace — full-window DockSpace with main menu bar
	// ============================================================
	void EditorUI::DrawDockSpace()
	{
		// Full-screen invisible window as DockSpace host
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpaceWindow", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		// Create DockSpace
		ImGuiID dockspaceId = ImGui::GetID("ElaineEditorDockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

		// Setup default layout on first frame
		if (mFirstFrame)
		{
			mFirstFrame = false;
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

			// Split: left 20% | center 60% | right 20%
			ImGuiID dockLeft, dockCenter, dockRight;
			ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockCenter);
			ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.25f, &dockRight, &dockCenter);

			// Split center: top 75% viewport | bottom 25% console
			ImGuiID dockBottom;
			ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.25f, &dockBottom, &dockCenter);

			// Dock panels to their positions
			ImGui::DockBuilderDockWindow("Scene Hierarchy", dockLeft);
			ImGui::DockBuilderDockWindow("Viewport", dockCenter);
			ImGui::DockBuilderDockWindow("Inspector", dockRight);
			ImGui::DockBuilderDockWindow("Console", dockBottom);
			ImGui::DockBuilderDockWindow("Content Browser", dockBottom);

			ImGui::DockBuilderFinish(dockspaceId);
		}

		// Draw menu bar inside DockSpace window
		DrawMenuBar();

		ImGui::End();
	}

	// ============================================================
	// DrawMenuBar
	// ============================================================
	void EditorUI::DrawMenuBar()
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene"))  {}
				if (ImGui::MenuItem("Open Scene")) {}
				if (ImGui::MenuItem("Save Scene")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Import Mesh..."))
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
						std::filesystem::path srcFs(srcPath);
						std::string destRelPath = "model/" + srcFs.stem().string() + ".emesh";
						std::filesystem::path modelDir = std::filesystem::path(Elaine::Root::instance()->GetResourcePath()) / "model";
						if (!std::filesystem::exists(modelDir))
							std::filesystem::create_directories(modelDir);
#ifdef _HAS_EDITOR_
						Elaine::ImportMeshFromFile(srcPath, destRelPath);
#endif
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
				{
					PostQuitMessage(0);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
				if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Preferences")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				for (auto* panel : mPanels)
				{
					if (ImGui::MenuItem(panel->GetTitle(), nullptr, panel->IsVisible()))
					{
						panel->ToggleVisible();
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("About Elaine Engine")) {}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	// ============================================================
	// Draw — main draw entry point
	// ============================================================
	void EditorUI::Draw()
	{
		DrawDockSpace();

		for (auto* panel : mPanels)
		{
			if (panel->IsVisible())
			{
				ImGui::Begin(panel->GetTitle(), panel->GetVisiblePtr());
				panel->OnDraw();
				ImGui::End();
			}
		}
	}
}
