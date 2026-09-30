#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineReflectionDefines.h"
#include "ElaineComponent.generated.h"

namespace Elaine
{
	class ElaineEngineExport ActorComponentInfo
	{
	public:
		ActorComponentInfo();
		virtual ~ActorComponentInfo();
		void				ExportData(JsonCpp& InJson);
		void				ImportData(const JsonCpp& InJson);
		virtual void		ExportDataImpl(JsonCpp& InJson);
		virtual void		ImportDataImpl(const JsonCpp& InJson);
	public:
		Name		mType;
		std::string	mGUID;
	};

	class Actor;
	class World;

	ECLASS()
	class ElaineEngineExport ActorComponent
	{
		GENERATED_BODY()
		friend class Actor;
	public:
		ActorComponent(Actor* InObject);
		virtual ~ActorComponent();
		void				Initialize(ActorComponentInfo* info);
		const std::string&	GetName() const { return mName; }
		Actor*			GetActor() { return mParent; }
		void				OnRegisterWorld(World* InWorld);
		void				OnUnregisterWorld();
		bool				GetVisible() const { return mbVisible; }
		EFUNCTION()
		void				SetVisible(bool InVisible);
		virtual const Name& GetType() const = 0;
		//--------------- ActorComponent Virtual Functions--------------------
		virtual void		OnCreate() { };
		virtual void		OnDestroy() { };
		virtual void		OnUpdate(float DeltaTime) { };
		virtual void		OnRegisterWorldImpl(World* InWorld) { }
		virtual void		OnUnregisterWorldImpl() { }
	protected:
		EPROPERTY(DisplayName="Visible", Category="ActorComponent", Tooltip="Whether the ActorComponent is visible")
		bool			mbVisible = true;
		Actor*		mParent = nullptr;
		ActorComponentInfo*	mDescription = nullptr;
		EPROPERTY(DisplayName="Name", Category="ActorComponent")
		std::string		mName;
		World*			mWorld = nullptr;
	};
}