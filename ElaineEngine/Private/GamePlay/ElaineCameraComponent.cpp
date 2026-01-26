#include "ElainePrecompiledHeader.h"
#include "ElaineCameraComponent.h"
#include "ElaineCamera.h"
#include "ElaineSceneManager.h"
#include "ElaineRenderCommandQueue.h"

namespace Elaine
{
	CameraComponent::CameraComponent(GameObject* InObject)
		: Component(InObject)
	{
	}

	CameraComponent::~CameraComponent()
	{
	}

	const Name& CameraComponent::GetType() const
	{
		static Name NameType("CameraComponent");
		return NameType;
	}

	void CameraComponent::OnCreate()
	{
		// 在渲染线程创建 Camera 对象
		String CameraName = "CameraComponent_Camera";
		ENQUEUE_RENDER_COMMAND(CreateCamera)([this, CameraName](RenderContext& Context)
		{
			mRenderCamera = new Camera(CameraName);
			// 注册到 SceneManager（如果需要）
			// SceneManager::Get()->RegisterCamera(mRenderCamera);
		});
	}

	void CameraComponent::OnDestroy()
	{
		// 在渲染线程销毁 Camera
		Camera* CameraToDelete = mRenderCamera;
		ENQUEUE_RENDER_COMMAND(DestroyCamera)([CameraToDelete](RenderContext& Context)
		{
			// SceneManager::Get()->UnregisterCamera(CameraToDelete);
			delete CameraToDelete;
		});
		mRenderCamera = nullptr;
	}

	void CameraComponent::OnUpdate(float DeltaTime)
	{
		// 每帧同步数据到渲染线程
		// 可以选择在这里批量同步，或者在每个 Set 方法中立即同步
		// SendUpdateToRenderThread();
	}

	//=============================================================================
	// 逻辑线程接口实现
	//=============================================================================
	void CameraComponent::SetPosition(const Vector3& Position)
	{
		mPosition = Position;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetRotation(const Vector3& Rotation)
	{
		mRotation = Quaternion::fromEulerZYX(Rotation);
		UpdateLocalAxes();
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetRotation(const Quaternion& Rotation)
	{
		mRotation = Rotation;
		UpdateLocalAxes();
		SendUpdateToRenderThread();
	}

	void CameraComponent::LookAt(const Vector3& Target)
	{
		Vector3 Direction = (Target - mPosition).normalisedCopy();
		// 计算旋转（简化实现）
		mForward = Direction;
		UpdateLocalAxes();
		
		Vector3 TargetCopy = Target;
		Camera* RenderCam = mRenderCamera;
		ENQUEUE_RENDER_COMMAND(CameraLookAt)([RenderCam, TargetCopy](RenderContext& Context)
		{
			if (RenderCam)
				RenderCam->LookAt(TargetCopy);
		});
	}

	void CameraComponent::SetFOV(float FOV)
	{
		mFOV = FOV;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetAspect(float Aspect)
	{
		mAspect = Aspect;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetNearPlane(float Near)
	{
		mNear = Near;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetFarPlane(float Far)
	{
		mFar = Far;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetProjectionType(ProjectionType Type)
	{
		mProjectionType = Type;
		SendUpdateToRenderThread();
	}

	//=============================================================================
	// 同步到渲染线程
	//=============================================================================
	void CameraComponent::SendUpdateToRenderThread()
	{
		// 捕获当前值（值拷贝，线程安全）
		Vector3 Pos = mPosition;
		Vector3 Rot = Quaternion::toEulerZYX(mRotation);
		float FOV = mFOV;
		float Near = mNear;
		float Far = mFar;
		float Aspect = mAspect;
		ProjectionType ProjType = mProjectionType;
		Camera* RenderCam = mRenderCamera;

		ENQUEUE_RENDER_COMMAND(UpdateCamera)([=](RenderContext& Context)
		{
			if (RenderCam)
			{
				RenderCam->SetPosition(Pos);
				RenderCam->SetRotation(Rot);
				RenderCam->SetFOV(FOV);
				// Camera 类没有 SetNearPlane/SetFarPlane，需要添加或使用其他方式
				RenderCam->SetAspect(Aspect);
				RenderCam->SetProjectionType(ProjType);
			}
		});
	}

	void CameraComponent::UpdateLocalAxes()
	{
		// 根据旋转更新局部坐标轴
		mForward = mRotation * Vector3::UNIT_Z;
		mUp = mRotation * Vector3::UNIT_Y;
		mRight = mRotation * Vector3::UNIT_X;
	}
}
