#include "ElaineEditorInput.h"
#include "ElaineViewportPanel.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Editor
{
    bool IsEditorUIInputActive()
    {
        const auto& Context = *ImGui::GetCurrentContext();
        return Context.ActiveId != 0 &&
            (!Context.ActiveIdWindow || Context.ActiveId != Context.ActiveIdWindow->MoveId);
    }

    void UpdateEditorInputRouting(ViewportPanel& Viewport, const Elaine::InputLayerHandle& UILayer, bool PopupBeforeFrame)
    {
        auto* Input = Elaine::InputSystem::instance();
        Viewport.UpdateInputLayers();
        const auto& IO = ImGui::GetIO();
        const bool Popup = PopupBeforeFrame || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        const bool WidgetActive = IsEditorUIInputActive();
        Elaine::InputLayerDesc UI;
        UI.Name = "EditorUI"; UI.Priority = 1000; UI.Modal = Popup;
        UI.KeyboardFocused = Popup || IO.WantTextInput || WidgetActive || (!Viewport.WantsKeyboard() && !Viewport.IsDragging());
        UI.Propagation.fill(Elaine::InputPropagation::Block);
        UI.HitTest = [&Viewport, Popup, WidgetActive](const Elaine::InputEvent& Event) {
            return Popup || WidgetActive || !Viewport.ContainsInput(Event);
        };
        Input->UpdateLayer(UILayer, std::move(UI));
    }
}
