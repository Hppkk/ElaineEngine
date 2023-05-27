#pragma once
#include "common/ElaineEnginePrerequirements.h"
#include "ElaineStdRequirements.h"

namespace Elaine
{
	class EGameObject;
	class ElaineEngineExport EComponent
	{
	public:
		EComponent();
		virtual ~EComponent();
		std::string&					GetName() { return m_sName; }
		EGameObject*					GetGameObject() { return m_pParentGameObject; }
	protected:
		std::string						m_sName;
		std::string						m_sGUID;
		EGameObject*					m_pParentGameObject;
	};
}