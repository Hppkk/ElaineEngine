#pragma once
#include "ElaineCorePrerequirements.h"
#include "math/ElaineAxisAlignedBox.h"

namespace Elaine
{
#define MAX_DEPTH  8
#define MIN_NODE_SIZE 100.0f  // 节点最小尺寸

	class RenderProxy;
	class Camera;
	class QuadTreeNode;

	enum class FrustumPlane
	{
		Near = 0,
		Far = 1,
		Left = 2,
		Right = 3,
		Top = 4,
		Bottom = 5,
		Count = 6
	};

	struct FrustumPlaneData
	{
		Vector3 normal;
		float distance;
		
		FrustumPlaneData() : normal(Vector3::ZERO), distance(0.0f) {}
		FrustumPlaneData(const Vector3& InNormal, float InDistance) 
			: normal(InNormal), distance(InDistance) {}
		
		float GetDistance(const Vector3& point) const 
		{
			return normal.dotProduct(point) + distance;
		}
	};

	struct ViewFrustum
	{
		FrustumPlaneData planes[(int)FrustumPlane::Count];
		
		// 从相机矩阵构建视锥体
		void BuildFromCamera(const Matrix4x4& viewProjMatrix);
		
		// 检查AABB是否在视锥体内
		bool IsAABBInFrustum(const AxisAlignedBox& aabb) const;
		
		// 检查点是否在视锥体内
		bool IsPointInFrustum(const Vector3& point) const;
	};

	// RenderProxy的上次位置缓存（用于增量更新）
	struct ProxyTransformCache
	{
		Matrix4x4 lastWorldMatrix;
		AxisAlignedBox lastWorldAABB;
		QuadTreeNode* lastNode;
		
		ProxyTransformCache() : lastNode(nullptr) {}
	};

	class ElaineCoreExport QuadTreeNode
	{
		friend class QuadTree;
	public:
		QuadTreeNode();
		~QuadTreeNode();
		
		void		AddRenderProxy(RenderProxy* InProxy);
		void		RemoveRenderProxy(RenderProxy* InProxy);
		
		// 获取当前节点范围
		const AxisAlignedBox& GetNodeBounds() const { return mNodeBounds; }
		
		// 获取当前节点所有渲染对象
		const std::vector<RenderProxy*>& GetRenderProxies() const { return mRenderProxys; }
		
		// 判断点是否在节点内
		bool IsPointInNode(const Vector3& point) const;
		
		// 判断AABB是否与节点相交
		bool IsAABBIntersect(const AxisAlignedBox& aabb) const;
		
		QuadTreeNode* GetChild(int index) const;
		bool HasChildren() const { return mRightTopNode != nullptr; }
		
		int GetDepth() const { return mdepth; }
		
	private:
		std::vector<RenderProxy*>	mRenderProxys;		
		AxisAlignedBox				mNodeBounds;
		QuadTreeNode*				mRightTopNode		= nullptr;
		QuadTreeNode*				mLeftTopNode		= nullptr;
		QuadTreeNode*				mRightBottomNode	= nullptr;
		QuadTreeNode*				mLeftBottomNode		= nullptr;
		QuadTreeNode*				mParentNode			= nullptr;
		int							mdepth				= 0;
	};

	class ElaineCoreExport QuadTree
	{
	public:
		QuadTree(const Vector3& InMin = Vector3(-1000, -1000, 0), 
				 const Vector3& InMax = Vector3(1000, 1000, 0));
		~QuadTree();
		
		void Initialize(const Vector3& InMin, const Vector3& InMax);
		
		QuadTreeNode*			CreateNode();
		void					AddRenderProxy(RenderProxy* InProxy);
		void					RemoveRenderProxy(RenderProxy* InProxy);
		
		// ===== 增量更新相关 =====
		// 更新RenderProxy的位置（增量更新，避免完全重新插入）
		void					UpdateRenderProxy(RenderProxy* InProxy);
		
		// 查找与AABB相交的所有渲染对象
		void FindIntersectingObjects(const AxisAlignedBox& queryAABB, std::vector<RenderProxy*>& outProxies);
		
		// ===== 视锥体剔除相关 =====
		// 使用视锥体查找可见对象
		void FindVisibleObjectsInFrustum(const ViewFrustum& frustum, std::vector<RenderProxy*>& outProxies);
		
		// 从相机构建视锥体并查找可见对象
		void FindVisibleObjectsByCamera(Camera* InCamera, std::vector<RenderProxy*>& outProxies);
		
		// 更新视锥体（用于重复查询优化）
		void UpdateFrustum(Camera* InCamera);
		
		// 获取当前缓存的视锥体
		const ViewFrustum& GetCurrentFrustum() const { return m_cachedFrustum; }
		
		// 查找可见对象（视锥体剔除等）
		void FindVisibilityObject();
		
		const AxisAlignedBox& GetTreeBounds() const { return m_TreeBounds; }
		QuadTreeNode* GetRootNode() const { return m_rootNode; }
		
	private:
		void InsertRenderProxyRecursive(QuadTreeNode* node, RenderProxy* proxy);
		
		void RemoveRenderProxyRecursive(QuadTreeNode* node, RenderProxy* proxy);
		
		void GetIntersectingObjectsRecursive(QuadTreeNode* node, const AxisAlignedBox& queryAABB, 
											 std::vector<RenderProxy*>& outProxies);

		void GetVisibleObjectsInFrustumRecursive(QuadTreeNode* node, const ViewFrustum& frustum,
												  std::vector<RenderProxy*>& outProxies);
		
		void SubdivideNode(QuadTreeNode* node);
		
		void TryMergeNode(QuadTreeNode* node);

	private:
		QuadTreeNode*			m_rootNode = nullptr;
		AxisAlignedBox			m_TreeBounds;		// 整个四叉树的边界
		int						m_depth	= 0;
		int						m_maxDepth = MAX_DEPTH;
		float					m_minNodeSize = MIN_NODE_SIZE;
		
		// ===== 增量更新缓存 =====
		std::unordered_map<RenderProxy*, ProxyTransformCache> m_proxyTransformCache;	// 代理对象的位置缓存
		
		// ===== 视锥体缓存 =====
		ViewFrustum				m_cachedFrustum;	// 缓存的视锥体
		Camera*					m_cachedCamera = nullptr;	// 缓存的相机指针
	};
}