#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ElaineCoreExport EComponentInfo
	{
	public:
		EComponentInfo();
		~EComponentInfo();
		void					exportData(cJSON* jsonNode);
		void					importData(cJSON* jsonNode);
		virtual void			exportDataImpl(cJSON* jsonNode);
		virtual void			importDataImpl(cJSON* jsonNode);
	private:
		std::string				m_sGUID;
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
		EGameObject*					m_pParentGameObject = nullptr;
		EComponentInfo*					m_pComInfo = nullptr;
		static std::string				m_sType;
	};
}