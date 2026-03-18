#include "ElaineEditorBase.h"

namespace Editor
{
    EditorPanel::EditorPanel(const char* title, bool visible)
		: mTitle(title), mVisible(visible)
	{
		mContext = EditorGlobalContext::instance();
	}
}