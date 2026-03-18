#pragma once
#include "GamePlay/ElaineComponent.h"
#include "math/ElaineTransform.h"
#include "ElaineComponentFactory.h"
#include "ElaineTickManager.h"
#include "ElaineReflectionDefines.h"
#include "ElaineTransformComponent.generated.h"

struct cJSON;
namespace Elaine
{
	class ElaineEngineExport TransformComponentInfo :public ComponentInfo
	{
	public:
		TransformComponentInfo();
		virtual ~TransformComponentInfo();
		virtual void		ExportDataImpl(JsonCpp& InJson) override;
		virtual void		ImportDataImpl(const JsonCpp& InJson) override;
	public:
		Transform*			m_pTransform;
	};

	ECLASS(DisplayName = "Transform")
	class ElaineEngineExport TransformComponent :public Component
	{
		GENERATED_BODY()
		friend class GameObject;
	public:
		TransformComponent(GameObject* InObject);
		virtual ~TransformComponent();
		const Matrix4x4& GetWorldMatrix() const;
		const Vector3& GetWorldScale() const;
		const Vector3& GetWorldPosition() const;
		const Quaternion& GetWorldRotation() const;
		const Vector3& GetScale() const;
		const Vector3& GetPosition() const;
		const Quaternion&GetRotation() const;
		EFUNCTION(DisplayName="Set Scale", Category="Transform")
		void SetScale(const Vector3& InScale);
		EFUNCTION(DisplayName="Set Position", Category="Transform")
		void SetPosition(const Vector3& InPosition);
		EFUNCTION(DisplayName="Set Rotation", Category="Transform")
		void SetRotation(const Quaternion& InRotation);
		void NodeTick(float InDeltaTime);
		void OnRegisterWorldImpl(World* InWorld) override;
		void OnUnregisterWorldImpl() override;
		const Name& GetType() const override;
	protected:
		EPROPERTY(DisplayName="Local Transform", Category="Transform", Tooltip="Local space transform")
		Transform mTransform;
		EPROPERTY(DisplayName="World Transform", Category="Transform", ReadOnly, Tooltip="World space transform (computed)")
		Transform mWorldTransform;
		TComponentTickTask<TransformComponent> mNodeTick;
	};

	DEFINE_COM_FACTORY(TransformComponent);
}