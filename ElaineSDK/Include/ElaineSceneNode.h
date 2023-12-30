#pragma once
#include "math/ElaineAxisAlignedBox.h"
#include "math/ElaineVector3.h"

namespace Elaine
{
	class RenderableObject;
	class EGameObject;

	class ElaineCoreExport SceneNode
	{
	public:
		SceneNode(SceneManager* mgr, String name = "");
		~SceneNode();
		AxisAlignedBox			getWorldAABB() const { return m_BoundingBox; }
		Vector3					getWorldPosition() const { return m_WorldPosition; }
		Vector3					getWorldScale() const { return m_WorldScale; }
		Vector3					getWorldRotation() const { return m_WorldRotation; }
		Quaternion				getWorldQuaternion() const { return m_WorldQuaternion; }
		void					setWorldPosition(const Vector3& pos);
		void					setWorldScale(const Vector3& scale);
		void					setWorldRotation(const Vector3& rotation);
		void					setWorldQuaternion(const Quaternion& rotation);
		void					update(bool updateChild = true, bool notifyParent = true);
		void					updateBindingBox();
		void					findVisibilityObject();
		SceneNode*				createChild();
		SceneManager*			getCreater() { return m_Creater; }
		void					attachChildNode(SceneNode* node);
		void					detachChildNode(SceneNode* node);
		void					setVisible(bool vis) { m_Visibile = vis; }
		bool					isVisible() { return m_Visibile; }
	private:								  

		AxisAlignedBox						m_BoundingBox;
		Vector3								m_WorldPosition;
		Vector3								m_WorldScale;
		Vector3								m_WorldRotation;
		Quaternion							m_WorldQuaternion;
		std::vector<RenderableObject*>		m_RenderObjects;
		std::vector<EGameObject*>			m_GemeObjectVec;
		SceneNode*							m_ParentNode = nullptr;
		std::set<SceneNode*>				m_ChildSet;
		SceneManager*						m_Creater = nullptr;
		String								m_Name;
		bool								m_Visibile = true;
	};
}