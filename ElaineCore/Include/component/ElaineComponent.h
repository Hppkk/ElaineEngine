#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ElaineCoreExport EComponentInfo
	{
	public:
		EComponentInfo();
		~EComponentInfo();
	private:
	};

	class EGameObject;
	class ElaineCoreExport EComponent
	{
	public:
		EComponent();
		virtual ~EComponent();
		void							init(EComponentInfo* info);
		std::string&					GetName() { return m_sName; }
		EGameObject*					GetGameObject() { return m_pParentGameObject; }
		std::string&					GetType() { return m_sType; }
	protected:
		std::string						m_sName;
		std::string						m_sGUID;
		EGameObject*					m_pParentGameObject;
		EComponentInfo*					m_pComInfo;
		std::string						m_sType;
	};
}