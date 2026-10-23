#pragma once
#include "ElaineInputSystem.h"

namespace Editor
{
    class ViewportPanel;
    // Window background move IDs are not active property/menu controls.
    bool IsEditorUIInputActive();
    // Called after drawing UI and collecting viewport conditions, before dispatch.
    void UpdateEditorInputRouting(ViewportPanel& Viewport, const Elaine::InputLayerHandle& UILayer, bool PopupBeforeFrame);
}
