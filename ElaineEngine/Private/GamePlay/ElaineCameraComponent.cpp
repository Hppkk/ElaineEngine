#include "ElainePrecompiledHeader.h"
#include "ElaineCameraComponent.h"
#include "ElaineCamera.h"
#include "ElaineSceneManager.h"
#include "ElaineRenderCommandQueue.h"

namespace Elaine
{
	CameraComponent::CameraComponent(Actor* InObject)
		: ActorComponent(InObject)
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
        Root::instance()->FlushRenderCommands();
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
    CameraState CameraComponent::CaptureState() const
    {
        CameraState State;
        State.Position = mPosition; State.Rotation = mRotation; State.FOV = mFOV;
        State.Near = mNear; State.Far = mFar; State.Aspect = mAspect; State.Projection = mProjectionType;
        return ValidateCameraState(State);
    }

    void CameraComponent::ApplyCameraState()
    {
        const auto State = CaptureState();
        mPosition = State.Position; mRotation = State.Rotation; mFOV = State.FOV;
        mNear = State.Near; mFar = State.Far; mAspect = State.Aspect; mProjectionType = State.Projection;
        mbCacheViewOutOfData = mbCacheOutOfData = true;
        UpdateLocalAxes();
        SendUpdateToRenderThread();
    }

    void CameraComponent::SetPosition(const Vector3& Value) { mPosition = Value; ApplyCameraState(); }
    void CameraComponent::SetRotation(const Vector3& Value) { SetRotation(Quaternion::fromEulerZYX(Value)); }
    void CameraComponent::SetRotation(const Quaternion& Value) { mRotation = Value; ApplyCameraState(); }
    void CameraComponent::LookAt(const Vector3& Target)
    {
        auto State = CaptureState();
        if (SetCameraLookAt(State, Target)) { mRotation = State.Rotation; ApplyCameraState(); }
    }
    void CameraComponent::SetFOV(float Value) { mFOV = Value; ApplyCameraState(); }
    void CameraComponent::SetAspect(float Value) { mAspect = Value; ApplyCameraState(); }
    void CameraComponent::SetNearPlane(float Value) { mNear = Value; ApplyCameraState(); }
    void CameraComponent::SetFarPlane(float Value) { mFar = Value; ApplyCameraState(); }
    void CameraComponent::SetProjectionType(ProjectionType Value) { mProjectionType = Value; ApplyCameraState(); }
    void CameraComponent::PostEditChangeProperty(const PropertyChangedEvent& Event)
    {
        ActorComponent::PostEditChangeProperty(Event);
        if (Event.Type != PropertyChangeType::EditFinished) ApplyCameraState();
    }

    void CameraComponent::SendUpdateToRenderThread()
    {
        Camera* RenderCamera = mRenderCamera;
        if (!RenderCamera) return;
        const auto State = CaptureState();
        ENQUEUE_RENDER_COMMAND(UpdateCamera)([RenderCamera, State](RenderContext&) { RenderCamera->SetState(State); });
    }

    void CameraComponent::UpdateLocalAxes()
    {
        mRight = mRotation * Vector3::UNIT_X;
        mUp = mRotation * Vector3::UNIT_Y;
        mForward = mRotation * Vector3::UNIT_Z;
    }
    void CameraComponent::UpdateViewParams() const
    {
        if (!mbCacheViewOutOfData && !mbCacheOutOfData) return;
        const auto Matrices = BuildCameraMatrices(CaptureState());
        mViewMatrix = Matrices.View; mProjMatrix = Matrices.Projection; mViewProjMatrix = Matrices.ViewProjection;
        mbCacheViewOutOfData = mbCacheOutOfData = false;
    }
    void CameraComponent::CalculateProjMatrix() const { UpdateViewParams(); }
    const Matrix4x4& CameraComponent::GetViewMatrix() const { UpdateViewParams(); return mViewMatrix; }
    const Matrix4x4& CameraComponent::GetProjMatrix() const { UpdateViewParams(); return mProjMatrix; }
    const Matrix4x4& CameraComponent::GetViewProjMatrix() const { UpdateViewParams(); return mViewProjMatrix; }
}
