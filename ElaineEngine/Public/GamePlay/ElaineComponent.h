#pragma once
#include "ElaineEnginePrerequirements.h"

namespace Elaine
{
	class ElaineEngineExport ComponentInfo
	{
	public:
		ComponentInfo();
		virtual ~ComponentInfo();
		void					ExportData(JsonCpp& InJson);
		void					ImportData(const JsonCpp& InJson);
		virtual void			ExportDataImpl(JsonCpp& InJson);
		virtual void			ImportDataImpl(const JsonCpp& InJson);
	public:
		Name			mType;
		std::string		mGUID;
	};

	class GameObject;
	class World;

	class ElaineEngineExport Component
	{
		friend class GameObject;
	public:
		Component(GameObject* InObject);
		virtual ~Component();
		void				Initialize(ComponentInfo* info);
		const std::string&	GetName() const { return mName; }
		GameObject*			GetGameObject() { return mParent; }
		void				OnRegisterWorld(World* InWorld);
		void				OnUnregisterWorld();
		bool				GetVisible() const { return mbVisible; }
		void				SetVisible(bool InVisible);
		virtual const Name& GetType() const = 0;
		//--------------- Component Virtual Functions--------------------
		virtual void		OnCreate() { };
		virtual void		OnDestroy() { };
		virtual void		OnUpdate(float DeltaTime) { };
		virtual void		OnRegisterWorldImpl(World* InWorld) { }
		virtual void		OnUnregisterWorldImpl() { }
	protected:
		bool			mbVisible = true;
		GameObject*		mParent = nullptr;
		ComponentInfo*	mDescription = nullptr;
		std::string		mName;
		World*			mWorld = nullptr;
	};
}