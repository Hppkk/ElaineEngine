#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineTransformComponent.h"
#include "math/ElaineTransform.h"
#include "ElaineWorld.h"
#include "ElaineCameraState.h"
#include "math/ElaineDynamicBVH.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "GamePlay/ElaineActor.h"

namespace Elaine
{
	TransformComponentInfo::TransformComponentInfo()
	{
		m_pTransform = new Transform();
	}

	TransformComponentInfo::~TransformComponentInfo()
	{
		SAFE_DELETE(m_pTransform);
	}

	void TransformComponentInfo::ExportDataImpl(JsonCpp& InJson)
	{
		InJson["Position"] = JsonCpp::array({ m_pTransform->mPosition.x,m_pTransform->mPosition.y,m_pTransform->mPosition.z });
		InJson["Scale"] = JsonCpp::array({ m_pTransform->mScale.x, m_pTransform->mScale.y, m_pTransform->mScale.z });
		InJson["Rotation"] = JsonCpp::array({ m_pTransform->mRotation.x, m_pTransform->mRotation.y, m_pTransform->mRotation.z, m_pTransform->mRotation.w });
	}

	void TransformComponentInfo::ImportDataImpl(const JsonCpp& InJson)
	{
		m_pTransform->mPosition.x = InJson["Position"][0];
		m_pTransform->mPosition.y = InJson["Position"][1];
		m_pTransform->mPosition.z = InJson["Position"][2];

		m_pTransform->mScale.x = InJson["Scale"][0];
		m_pTransform->mScale.y = InJson["Scale"][1];
		m_pTransform->mScale.z = InJson["Scale"][2];

		m_pTransform->mRotation.x = InJson["Rotation"][0];
		m_pTransform->mRotation.y = InJson["Rotation"][1];
		m_pTransform->mRotation.z = InJson["Rotation"][2];
		m_pTransform->mRotation.w = InJson["Rotation"][3];
		
	}

	TransformComponent::TransformComponent(Actor* InObject)
		: ActorComponent(InObject)
	{
		//RegisterTickTime(BeginFrame);
		//RegisterComNeedTick();
		mNodeTick.Bind(this, &TransformComponent::NodeTick, TickGroup::LateUpdate);
	}

	TransformComponent::~TransformComponent()
	{
		
	}

    void TransformComponent::UpdateWorldTransform() const
    {
        Matrix4x4 World = mTransform.GetMatrix();
        if (mParent && mParent->GetParent()) World = mParent->GetParent()->GetWorldMatrix() * World;
        World.decomposition(mWorldTransform.mPosition, mWorldTransform.mScale, mWorldTransform.mRotation);
        mWorldTransform.mMatrix = World; mWorldTransform.mDirty = false;
    }
    const Matrix4x4& TransformComponent::GetWorldMatrix() const { UpdateWorldTransform(); return mWorldTransform.mMatrix; }
    const Vector3& TransformComponent::GetWorldScale() const { UpdateWorldTransform(); return mWorldTransform.mScale; }
    const Vector3& TransformComponent::GetWorldPosition() const { UpdateWorldTransform(); return mWorldTransform.mPosition; }
    const Quaternion& TransformComponent::GetWorldRotation() const { UpdateWorldTransform(); return mWorldTransform.mRotation; }
    const Vector3& TransformComponent::GetScale() const { return mTransform.mScale; }
    const Vector3& TransformComponent::GetPosition() const { return mTransform.mPosition; }
    const Quaternion& TransformComponent::GetRotation() const { return mTransform.mRotation; }
    void TransformComponent::SetScale(const Vector3& Value) { mTransform.mScale = Value; ApplyTransformChange(); }
    void TransformComponent::SetPosition(const Vector3& Value) { mTransform.mPosition = Value; ApplyTransformChange(); }
    void TransformComponent::SetRotation(const Quaternion& Value) { mTransform.mRotation = NormalizeCameraRotation(Value); ApplyTransformChange(); }
    void TransformComponent::ApplyTransformChange()
    {
        mTransform.mRotation = NormalizeCameraRotation(mTransform.mRotation);
        mTransform.mDirty = true;
        if (mParent) mParent->UpdateNode(true, false);
    }
    void TransformComponent::PostEditChangeProperty(const PropertyChangedEvent& Event)
    {
        ActorComponent::PostEditChangeProperty(Event);
        if (Event.IsProperty("mTransform") && Event.Type != PropertyChangeType::EditFinished) ApplyTransformChange();
    }
    void TransformComponent::NodeTick(float)
    {
        UpdateWorldTransform();
        if (!mParent) return;
        for (const auto& Pair : mParent->GetComponents())
            if (auto* Mesh = dynamic_cast<StaticMeshComponent*>(Pair.second)) Mesh->MarkTransformDirty();
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(mParent);
    }

	void TransformComponent::OnRegisterWorldImpl(World* InWorld)
	{
		InWorld->RegisterTickTask(&mNodeTick);
	}

	void TransformComponent::OnUnregisterWorldImpl()
	{
		mWorld->UnregisterTickTask(&mNodeTick);
	}

	const Name& TransformComponent::GetType() const
	{
		static Name NameType("TransformComponent");
		return NameType;
	}

}
