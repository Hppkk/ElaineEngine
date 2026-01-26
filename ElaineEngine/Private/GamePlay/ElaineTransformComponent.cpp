#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineTransformComponent.h"
#include "math/ElaineTransform.h"
#include "ElaineWorld.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "GamePlay/ElaineGameObject.h"

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
		InJson["Position"].array({ m_pTransform->mPosition.x,m_pTransform->mPosition.y,m_pTransform->mPosition.z });
		InJson["Scale"].array({ m_pTransform->mScale.x, m_pTransform->mScale.y, m_pTransform->mScale.z });
		InJson["Rotation"].array({ m_pTransform->mRotation.x, m_pTransform->mRotation.y, m_pTransform->mRotation.z, m_pTransform->mRotation.w });
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

	TransformComponent::TransformComponent(GameObject* InObject)
		: Component(InObject)
	{
		//RegisterTickTime(BeginFrame);
		//RegisterComNeedTick();
		mNodeTick.Bind(this, &TransformComponent::NodeTick, TickGroup::LateUpdate);
	}

	TransformComponent::~TransformComponent()
	{
		
	}

	const Matrix4x4& TransformComponent::GetWorldMatrix() const
	{
		return mWorldTransform.GetMatrix();
	}

	const Vector3& TransformComponent::GetWorldScale() const
	{
		return mTransform.mScale;
	}

	const Vector3& TransformComponent::GetWorldPosition() const
	{
		return mWorldTransform.mPosition;
	}

	const Quaternion& TransformComponent::GetWorldRotation() const
	{
		return mWorldTransform.mRotation;
	}

	const Vector3& TransformComponent::GetScale() const
	{
		return mTransform.mScale;
	}

	const Vector3& TransformComponent::GetPosition() const
	{
		return mTransform.mPosition;
	}

	const Quaternion& TransformComponent::GetRotation() const
	{
		return mTransform.mRotation;
	}

	void TransformComponent::SetScale(const Vector3& InScale)
	{
		mTransform.mScale = InScale;
	}
	void TransformComponent::SetPosition(const Vector3& InPosition)
	{
		mTransform.mPosition = InPosition;
	}
	void TransformComponent::SetRotation(const Quaternion& InRotation)
	{
		mTransform.mRotation = InRotation;
	}

	void TransformComponent::NodeTick(float InDeltaTime)
	{
		// Update world transform based on parent GameObject's transform (no SceneNode)
		if (mParent == nullptr)
			return;

		Matrix4x4 localMat = mTransform.GetMatrix();
		Matrix4x4 worldMat = localMat;

		GameObject* owner = mParent;
		GameObject* parentGO = owner->GetParent();
		if (parentGO)
		{
			Component* pcom = parentGO->GetComponentByName(Name("TransformComponent"));
			if (pcom)
			{
				TransformComponent* parentTransform = static_cast<TransformComponent*>(pcom);
				worldMat = parentTransform->GetWorldMatrix() * localMat;
			}
		}

		// Decompose world matrix to components
		Vector3 pos;
		Vector3 scale;
		Quaternion rot;
		worldMat.decomposition(pos, scale, rot);

		mWorldTransform.mMatrix = worldMat;
		mWorldTransform.mDirty = false;
		mWorldTransform.mPosition = pos;
		mWorldTransform.mScale = scale;
		mWorldTransform.mRotation = rot;

		// Notify mesh-like components on this GameObject that transform changed
		auto& comps = mParent->GetComponents();
		for (auto& kv : comps)
		{
			Component* com = kv.second;
			if (com == nullptr) continue;
			// dynamic_cast may be used to identify mesh components
			StaticMeshComponent* meshCom = nullptr;
			meshCom = dynamic_cast<StaticMeshComponent*>(com);
			if (meshCom)
			{
				meshCom->MarkTransformDirty();
			}
		}
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
