#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineComponent.h"
#include "math/ElaineVector3.h"
#include "ElaineCamera.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "ElaineReflectionDefines.h"
#include "ElaineCameraComponent.generated.h"

namespace Elaine
{
	class Camera;

	class ElaineEngineExport CameraComponentInfo : public ComponentInfo
	{
	public:
	};

	ECLASS(DisplayName = "Camera")
	class ElaineEngineExport CameraComponent : public Component
	{
		GENERATED_BODY()
	public:
		CameraComponent(GameObject* InObject);
		virtual ~CameraComponent();

		virtual void OnCreate() override;
		virtual void OnDestroy() override;
		virtual void OnUpdate(float DeltaTime) override;

		EFUNCTION(Category="Camera")
		void SetPosition(const Vector3& Position);
		EFUNCTION(Category="Camera")
		void SetRotation(const Vector3& Rotation);
		EFUNCTION(Category="Camera")
		void SetRotation(const Quaternion& Rotation);
		EFUNCTION(Category="Camera")
		void LookAt(const Vector3& Target);
		EFUNCTION(Category="Camera")
		void SetFOV(float FOV);
		EFUNCTION(Category="Camera")
		void SetAspect(float Aspect);
		EFUNCTION(Category="Camera")
		void SetNearPlane(float Near);
		EFUNCTION(Category="Camera")
		void SetFarPlane(float Far);
		EFUNCTION(Category="Camera")
		void SetProjectionType(ProjectionType Type);

		const Vector3& GetPosition() const { return mPosition; }
		const Quaternion& GetRotation() const { return mRotation; }
		const Vector3& GetForward() const { return mForward; }
		const Vector3& GetUp() const { return mUp; }
		const Vector3& GetRight() const { return mRight; }
		float GetFOV() const { return mFOV; }
		float GetAspect() const { return mAspect; }
		ProjectionType GetProjectionType() const { return mProjectionType; }
		const Name& GetType() const override;

		Camera* GetRenderThreadCamera() const { return mRenderCamera; }

		const Matrix4x4& GetViewMatrix() const;
		const Matrix4x4& GetProjMatrix() const;
		const Matrix4x4& GetViewProjMatrix() const;

	private:
		EPROPERTY(DisplayName="Position", Category="Camera", Tooltip="Camera world position")
		Vector3 mPosition = Vector3::ZERO;
		EPROPERTY(DisplayName="Rotation", Category="Camera", Tooltip="Camera rotation quaternion")
		Quaternion mRotation = Quaternion::IDENTITY;
		Vector3 mForward = Vector3::UNIT_Z;
		Vector3 mUp = Vector3::UNIT_Y;
		Vector3 mRight = Vector3::UNIT_X;
		EPROPERTY(DisplayName="FOV", Category="Camera", Tooltip="Field of view in degrees", Min=1.0, Max=179.0)
		float mFOV = 60.0f;
		EPROPERTY(DisplayName="Near Plane", Category="Camera", Tooltip="Near clipping plane distance", Min=0.01, Max=100.0)
		float mNear = 0.1f;
		EPROPERTY(DisplayName="Far Plane", Category="Camera", Tooltip="Far clipping plane distance", Min=10.0, Max=100000.0)
		float mFar = 1000.0f;
		EPROPERTY(DisplayName="Aspect Ratio", Category="Camera", Tooltip="Width/Height ratio")
		float mAspect = 16.0f / 9.0f;
		EPROPERTY(DisplayName="Projection", Category="Camera", Tooltip="Projection type")
		ProjectionType mProjectionType = ProjectionType::Prespective;

		Camera* mRenderCamera = nullptr;

		mutable Matrix4x4 mViewMatrix{ Matrix4x4::IDENTITY };
		mutable Matrix4x4 mProjMatrix{ Matrix4x4::IDENTITY };
		mutable Matrix4x4 mViewProjMatrix{ Matrix4x4::IDENTITY };
		mutable bool mbCacheViewOutOfData = true;
		mutable bool mbCacheOutOfData = true;

		void SendUpdateToRenderThread();
		void UpdateLocalAxes();
		void UpdateViewParams() const;
		void CalculateProjMatrix() const;
	};

	DEFINE_COM_FACTORY(CameraComponent);
}
