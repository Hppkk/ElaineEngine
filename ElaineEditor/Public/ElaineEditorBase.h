#pragma once

namespace Editor
{
	class EditorManagerBase;

	class EditorBase
	{
	public:
		EditorBase(EditorManagerBase* InOwner);
		virtual ~EditorBase();
		EditorManagerBase* GetOwner() const { return mOwner; }
	private:
	protected:
		EditorManagerBase* mOwner;
	};
}