#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineReflectionDefines.h"
#include "ElaineComponent.generated.h"

namespace Elaine
{
	class ElaineEngineExport ComponentInfo
	{
	public:
		ComponentInfo();
		virtual ~ComponentInfo();
		void				ExportData(JsonCpp& InJson);
		void				ImportData(const JsonCpp& InJson);
		virtual void		ExportDataImpl(JsonCpp& InJson);
		virtual void		ImportDataImpl(const JsonCpp& InJson);
	public:
		Name		mType;
		std::string	mGUID;
	};

	class GameObject;
	class World;

	ECLASS()
	class ElaineEngineExport Component
	{
		GENERATED_BODY()
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
		EFUNCTION()
		void				SetVisible(bool InVisible);
		virtual const Name& GetType() const = 0;
		//--------------- Component Virtual Functions--------------------
		virtual void		OnCreate() { };
		virtual void		OnDestroy() { };
		virtual void		OnUpdate(float DeltaTime) { };
		virtual void		OnRegisterWorldImpl(World* InWorld) { }
		virtual void		OnUnregisterWorldImpl() { }
	protected:
		EPROPERTY(DisplayName="Visible", Category="Component", Tooltip="Whether the component is visible")
		bool			mbVisible = true;
		GameObject*		mParent = nullptr;
		ComponentInfo*	mDescription = nullptr;
		EPROPERTY(DisplayName="Name", Category="Component")
		std::string		mName;
		World*			mWorld = nullptr;
	};
}