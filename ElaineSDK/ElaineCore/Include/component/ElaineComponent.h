#pragma once
#include "ElaineCorePrerequirements.h"

struct cJSON;
namespace Elaine
{
	enum TickTime
	{
		beginFrame,
		fixedUpdate,
		animatorUpdate,
		endFrame,
	};

	class ElaineCoreExport EComponentInfo
	{
	public:
		EComponentInfo();
		virtual ~EComponentInfo();
		void					exportData(cJSON* jsonNode);
		void					importData(cJSON* jsonNode);
		virtual void			exportDataImpl(cJSON* jsonNode);
		virtual void			importDataImpl(cJSON* jsonNode);
	public:
		std::string				m_sGUID;
		std::string				m_sType;
	};

	class EGameObject;
	class ElaineCoreExport EComponent
	{
		friend class EGameObject;
		friend class ComponentTickManager;
	public:
		EComponent();
		virtual ~EComponent();
		void							init(EComponentInfo* info);
		std::string&					GetName() { return m_sName; }
		EGameObject*					GetGameObject() { return m_pParentGameObject; }
		std::string&					GetType() { return m_sType; }
	protected:
		virtual void					beginFrameTick(float dt){}
		virtual void					fixedUpdate(float dt){}
		virtual void					animatorUpdate(float dt){}
		virtual void					endFrame(float dt){}
		void							registerTickTime(TickTime tt);
		void							registerComNeedTick();
	protected:
		std::string						m_sName;
		EGameObject*					m_pParentGameObject = nullptr;
		EComponentInfo*					m_pComInfo = nullptr;
		static std::string				m_sType;
		std::set<TickTime>				m_tickTimeSet;
		bool							m_bIsTickCom = false;
	};

	class ElaineCoreExport ComponentTickManager :public Singleton<ComponentTickManager>
	{
	public:
		ComponentTickManager() = default;
		~ComponentTickManager();
		void			registerTickComponent(EComponent*);
		void			unRegisterTickComponent(EComponent*);
		void			tick(float dt);
	private:
		std::set<EComponent*>		m_components;
	};
}