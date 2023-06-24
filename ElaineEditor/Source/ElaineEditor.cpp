#include "ElaineEditor.h"
#include "ElaineLogSystem.h"

namespace Elaine
{
	ElaineEditor::ElaineEditor()
	{

	}

	ElaineEditor::~ElaineEditor()
	{
		close();
	}

	void ElaineEditor::initialize()
	{
		LOG_INFO("Editor Init");
	}

	void ElaineEditor::close()
	{
		LOG_INFO("Editor Close");
	}

	void ElaineEditor::tick(float dt)
	{

	}
}