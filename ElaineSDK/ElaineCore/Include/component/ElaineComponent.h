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
		friend class EGameObject;
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
		EGameObject*					m_pParentGameObject = nullptr;
		EComponentInfo*					m_pComInfo = nullptr;
		static std::string				m_sType;
	};
}