#pragma once
#include "ElaineEnginePrerequirements.h"
#include "GamePlay/ElaineComponent.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "ElaineMesh.h"
#include "ElaineReflectionDefines.h"
#include "ElaineMeshComponent.generated.h"

namespace Elaine
{
	class MaterialInstanceDynamic;
	class StaticMeshRenderProxy;

	class ElaineEngineExport StaticMeshComponentInfo : public ComponentInfo
	{
	public:
	public:
		std::string mPath;
	};

	ECLASS(DisplayName = "Static Mesh")
	class ElaineEngineExport StaticMeshComponent : public Component
	{
		GENERATED_BODY()
	public:
		StaticMeshComponent(GameObject* InObject);
		virtual ~StaticMeshComponent();
		EFUNCTION(Category="Mesh")
		void ChangeMesh(const std::string& InPath);
		EFUNCTION(Category="Mesh")
		void ChangeMaterial(uint32 Index, const std::string& InMatName);
		void OnRegisterWorldImpl(World* InWorld) override;
		void OnUnregisterWorldImpl() override;
		const Name& GetType() const override;
        void MarkRenderStateDirty();
        void MarkTransformDirty();
	private:
		StaticMeshRenderProxy* mRenderProxy = nullptr;
		std::vector<MaterialInstanceDynamic*> mMaterials;
		MeshPtr mMesh = nullptr;
		EPROPERTY(DisplayName="Cast Shadow", Category="Rendering", Tooltip="Whether this mesh casts shadows")
		bool mbCastShadow = true;
		EPROPERTY(DisplayName="Receive Shadow", Category="Rendering", Tooltip="Whether this mesh receives shadows")
		bool mbReceiveShadow = true;
		EPROPERTY(DisplayName="Render Layer", Category="Rendering", Tooltip="Render layer index", Min=0, Max=255)
		uint8 mRenderLayer = 0;
		EPROPERTY(DisplayName="Mesh Path", Category="Mesh", Tooltip="Path to the mesh resource")
		std::string mPath;
	};

	DEFINE_COM_FACTORY(StaticMeshComponent);
}