#pragma once
#include "ElaineEnginePrerequirements.h"
#include "GamePlay/ElaineComponent.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "ElaineMesh.h"

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

	class ElaineEngineExport StaticMeshComponent : public Component
	{
	public:
		StaticMeshComponent(GameObject* InObject);
		virtual ~StaticMeshComponent();
		void ChangeMesh(const std::string& InPath);
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
		bool mbCastShadow = true;
		bool mbReceiveShadow = true;
		uint8 mRenderLayer = 0;
		std::string mPath;
	};

	DEFINE_COM_FACTORY(StaticMeshComponent);
}