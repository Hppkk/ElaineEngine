#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	//多线程模式下仅在渲染线程调用
	class SceneNode;
	class SceneManager;
	class Material;
	class RenderQueueSet;
	class RHIBuffer;

	struct VertexData
	{
		float mVertex[3];
		float mNormals[3];
		float mUV[2];
	};

	struct RenderGeneralData
	{
		std::vector<VertexData> mVertexDatas;
		std::vector<uint16> mIndexes;
		RHIDrawData mDrawData;
	};

	class ElaineCoreExport RenderableObject
	{
	public:
		RenderableObject(SceneNode* InParent, SceneManager* InSceneMgr);
		~RenderableObject();
		const AxisAlignedBox&		GetWorldAABB();
		const AxisAlignedBox&		GetLocalAABB();
		const Vector3&				GetWorldPosition() const;
		const Vector3&				GetWorldScale() const;
		const Quaternion&			GetWorldRotation() const;
		const Matrix4x4&			GetWorldMatrix();
		const RenderGeneralData&	GetRenderGeneralData()const { return m_RenderGeneralData; }
		SceneNode*					GetParentNode() { return m_SceneNode; }
		void						SetWorldPosition(const Vector3& pos);
		void						SetWorldScale(const Vector3& scale);
		void						SetWorldRotation(const Quaternion& rotation);
		bool						IsVisible() const { return m_bVisible; }
		bool						IsCulled() const { return m_bIsCulled; }
		void						SetVisible(bool InVal);
		void						UpdateWorldMatrix();
		void						UpdateBound();
		virtual Material*			GetMaterial() = 0;
		virtual void				SynchRenderData() = 0;
		virtual void				NotifyCurrentCamera(Camera* InCamera) = 0;
		virtual void				UpdateRenderQueue(RenderQueueSet* InRenderQueueSet) = 0;
		virtual void				RecordRenderCommand(RHICommandList* InRHICommandList) = 0;
	protected:
		SceneNode*			m_SceneNode = nullptr;
		SceneManager*		m_SceneManager = nullptr;
		RenderGeneralData	m_RenderGeneralData;
		size_t				m_Index = -1;
		bool				m_bVisible = true;
		bool				m_bIsCulled = false;
		friend class SceneNode;
	};
}