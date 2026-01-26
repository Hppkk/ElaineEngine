#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineComponent.h"
#include "math/ElaineVector3.h"
#include "ElaineCamera.h"
#include "GamePlay/ElaineComponentFactory.h"

namespace Elaine
{
	class Camera; // 渲染线程对象

	class ElaineEngineExport CameraComponentInfo : public ComponentInfo
	{
	public:
	};

	//=============================================================================
	// CameraComponent - 逻辑线程相机组件
	// 镜像 Camera 的所有接口，通过 RenderCommand 同步到渲染线程
	//=============================================================================
	class ElaineEngineExport CameraComponent : public Component
	{
	public:
		CameraComponent(GameObject* InObject);
		virtual ~CameraComponent();

		//=========================================================================
		// Component 生命周期
		//=========================================================================
		virtual void OnCreate() override;
		virtual void OnDestroy() override;
		virtual void OnUpdate(float DeltaTime) override;

		//=========================================================================
		// 逻辑线程接口（镜像 Camera 的所有接口）
		//=========================================================================
		void SetPosition(const Vector3& Position);
		void SetRotation(const Vector3& Rotation);
		void SetRotation(const Quaternion& Rotation);
		void LookAt(const Vector3& Target);
		void SetFOV(float FOV);
		void SetAspect(float Aspect);
		void SetNearPlane(float Near);
		void SetFarPlane(float Far);
		void SetProjectionType(ProjectionType Type);

		//=========================================================================
		// 逻辑线程查询接口
		//=========================================================================
		const Vector3& GetPosition() const { return mPosition; }
		const Quaternion& GetRotation() const { return mRotation; }
		const Vector3& GetForward() const { return mForward; }
		const Vector3& GetUp() const { return mUp; }
		const Vector3& GetRight() const { return mRight; }
		float GetFOV() const { return mFOV; }
		float GetAspect() const { return mAspect; }
		const Name& GetType() const override;

		//=========================================================================
		// 渲染线程 Camera 访问（仅供渲染系统内部使用）
		//=========================================================================
		Camera* GetRenderThreadCamera() const { return mRenderCamera; }

	private:
		//=========================================================================
		// 逻辑线程数据（Game Thread 拥有）
		//=========================================================================
		Vector3 mPosition = Vector3::ZERO;
		Quaternion mRotation = Quaternion::IDENTITY;
		Vector3 mForward = Vector3::UNIT_Z;
		Vector3 mUp = Vector3::UNIT_Y;
		Vector3 mRight = Vector3::UNIT_X;
		float mFOV = 60.0f;
		float mNear = 0.1f;
		float mFar = 1000.0f;
		float mAspect = 16.0f / 9.0f;
		ProjectionType mProjectionType = ProjectionType::Prespective;

		//=========================================================================
		// 渲染线程 Camera 指针（只读，不要在 Game Thread 访问其成员）
		//=========================================================================
		Camera* mRenderCamera = nullptr;

		//=========================================================================
		// 辅助方法
		//=========================================================================
		void SendUpdateToRenderThread();
		void UpdateLocalAxes();
	};

	DEFINE_COM_FACTORY(CameraComponent);
}
