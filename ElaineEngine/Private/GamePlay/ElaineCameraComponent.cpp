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
		mbCacheViewOutOfData = true;
		mbCacheOutOfData = true;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetRotation(const Vector3& Rotation)
	{
		mRotation = Quaternion::fromEulerZYX(Rotation);
		mbCacheViewOutOfData = true;
		mbCacheOutOfData = true;
		UpdateLocalAxes();
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetRotation(const Quaternion& Rotation)
	{
		mRotation = Rotation;
		mbCacheViewOutOfData = true;
		mbCacheOutOfData = true;
		UpdateLocalAxes();
		SendUpdateToRenderThread();
	}

	void CameraComponent::LookAt(const Vector3& Target)
	{
		Vector3 Direction = (Target - mPosition).normalisedCopy();
		// 计算旋转（简化实现）
		mForward = Direction;
		UpdateLocalAxes(); // Here we technically should update mRotation from Direction, simplified for now
		mbCacheViewOutOfData = true;
		mbCacheOutOfData = true;
		
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
		mbCacheOutOfData = true;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetAspect(float Aspect)
	{
		mAspect = Aspect;
		mbCacheOutOfData = true;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetNearPlane(float Near)
	{
		mNear = Near;
		mbCacheOutOfData = true;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetFarPlane(float Far)
	{
		mFar = Far;
		mbCacheOutOfData = true;
		SendUpdateToRenderThread();
	}

	void CameraComponent::SetProjectionType(ProjectionType Type)
	{
		mProjectionType = Type;
		mbCacheOutOfData = true;
		SendUpdateToRenderThread();
	}

	//=============================================================================
	// 同步到渲染线程
	//=============================================================================
	void CameraComponent::SendUpdateToRenderThread()
	{
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

	const Matrix4x4& CameraComponent::GetViewMatrix() const
	{
		if (mbCacheViewOutOfData)
		{
			UpdateViewParams();
		}
		return mViewMatrix;
	}

	const Matrix4x4& CameraComponent::GetProjMatrix() const
	{
		if (mbCacheViewOutOfData)
		{
			UpdateViewParams();
		}
		else if (mbCacheOutOfData)
		{
			CalculateProjMatrix();
		}
		return mProjMatrix;
	}

	const Matrix4x4& CameraComponent::GetViewProjMatrix() const
	{
		if (mbCacheViewOutOfData)
		{
			UpdateViewParams();
		}
		else if (mbCacheOutOfData)
		{
			CalculateProjMatrix();
		}
		return mViewProjMatrix;
	}

	void CameraComponent::UpdateViewParams() const
	{
		if (!mbCacheViewOutOfData) return;

		Matrix4x4 TranslationMat = Matrix4x4::IDENTITY;
		TranslationMat[0][3] = -mPosition[0];
		TranslationMat[1][3] = -mPosition[1];
		TranslationMat[2][3] = -mPosition[2];

		// We assume mRight, mUp, mForward are up-to-date
		// Standard View matrix (Right dot Pos, etc. simplified using TranslationMat)
		mViewMatrix = Matrix4x4(mRight[0], mUp[0], mForward[0], 0.0f,
								mRight[1], mUp[1], mForward[1], 0.0f,
								mRight[2], mUp[2], mForward[2], 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f).transpose() * TranslationMat;

		mbCacheViewOutOfData = false;
		CalculateProjMatrix();
	}

	void CameraComponent::CalculateProjMatrix() const
	{
		if (!mbCacheOutOfData) return;
		mbCacheOutOfData = false;

		Degree Fovy(mFOV);
		float RadFovy = Fovy.ValueRadians();
		if (mProjectionType == ProjectionType::Prespective)
		{
			float xx = 1.0f / (mAspect * Math::tan(RadFovy / 2.0f));
			float yy = 1.0f / Math::tan(RadFovy / 2.0f);
			float zz = mFar / (mFar - mNear);
			float zw = -(mFar * mNear) / (mFar - mNear);
			mProjMatrix = Matrix4x4(xx, 0.0f, 0.0f, 0.0f,
									0.0f, yy, 0.0f, 0.0f,
									0.0f, 0.0f, zz, zw,
									0.0f, 0.0f, 1.0f, 0.0f);
		}
		else
		{
			float h = 2 * mNear * Math::tan(RadFovy / 2.0f);
			float w = mAspect * h;
			float l = -w / 2.0f;
			float r = w / 2.0f;
			float b = -h / 2.0f;
			float t = h / 2.0f;
			mProjMatrix = Matrix4x4(2.0f / (r - l), 0.0f, 0.0f, -(r + l) / (r - l),
									0.0f, -2.0f / (t - b), 0.0f, -(t + b) / (t - b),
									0.0f, 0.0f, 1.0f / (mFar - mNear), -mNear / (mFar - mNear),
									0.0f, 0.0f, 0.0f, 1.0f);
		}

		mViewProjMatrix = mProjMatrix * mViewMatrix;
	}
}
