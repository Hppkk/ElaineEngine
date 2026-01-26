#include "ElainePrecompiledHeader.h"
#include "ElaineQuadTree.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "ElaineCamera.h"

namespace Elaine
{
	// ======================== ViewFrustum Implementation ========================
	
	void ViewFrustum::BuildFromCamera(const Matrix4x4& viewProjMatrix)
	{
		// 从视图投影矩阵提取视锥体平面
		// 这是标准的视锥体提取算法
		
		Matrix4x4 m = viewProjMatrix;
		float* mat = (float*)&m;
		
		// Near plane: z + w = 0
		planes[(int)FrustumPlane::Near].normal = Vector3(
			mat[2], mat[6], mat[10]
		).normalisedCopy();
		planes[(int)FrustumPlane::Near].distance = mat[14];
		
		// Far plane: -z + w = 0
		planes[(int)FrustumPlane::Far].normal = Vector3(
			-mat[2], -mat[6], -mat[10]
		).normalisedCopy();
		planes[(int)FrustumPlane::Far].distance = -mat[14];
		
		// Left plane: x + w = 0
		planes[(int)FrustumPlane::Left].normal = Vector3(
			mat[0], mat[4], mat[8]
		).normalisedCopy();
		planes[(int)FrustumPlane::Left].distance = mat[12];
		
		// Right plane: -x + w = 0
		planes[(int)FrustumPlane::Right].normal = Vector3(
			-mat[0], -mat[4], -mat[8]
		).normalisedCopy();
		planes[(int)FrustumPlane::Right].distance = -mat[12];
		
		// Top plane: -y + w = 0
		planes[(int)FrustumPlane::Top].normal = Vector3(
			-mat[1], -mat[5], -mat[9]
		).normalisedCopy();
		planes[(int)FrustumPlane::Top].distance = -mat[13];
		
		// Bottom plane: y + w = 0
		planes[(int)FrustumPlane::Bottom].normal = Vector3(
			mat[1], mat[5], mat[9]
		).normalisedCopy();
		planes[(int)FrustumPlane::Bottom].distance = mat[13];
	}
	
	bool ViewFrustum::IsAABBInFrustum(const AxisAlignedBox& aabb) const
	{
		const Vector3& min = aabb.getMin();
		const Vector3& max = aabb.getMax();
		
		// 检查AABB的8个顶点
		Vector3 vertices[8] = {
			Vector3(min.x, min.y, min.z),
			Vector3(max.x, min.y, min.z),
			Vector3(min.x, max.y, min.z),
			Vector3(max.x, max.y, min.z),
			Vector3(min.x, min.y, max.z),
			Vector3(max.x, min.y, max.z),
			Vector3(min.x, max.y, max.z),
			Vector3(max.x, max.y, max.z)
		};
		
		// 对每个平面进行检查
		for (int i = 0; i < (int)FrustumPlane::Count; ++i)
		{
			bool allOutside = true;
			for (int j = 0; j < 8; ++j)
			{
				if (planes[i].GetDistance(vertices[j]) >= 0)
				{
					allOutside = false;
					break;
				}
			}
			
			// 如果所有顶点都在平面外侧，则AABB在视锥体外
			if (allOutside)
				return false;
		}
		
		return true;
	}
	
	bool ViewFrustum::IsPointInFrustum(const Vector3& point) const
	{
		for (int i = 0; i < (int)FrustumPlane::Count; ++i)
		{
			if (planes[i].GetDistance(point) < 0)
				return false;
		}
		return true;
	}

	// ======================== QuadTreeNode Implementation ========================
	
	QuadTreeNode::QuadTreeNode()
		: mNodeBounds(Vector3::ZERO, Vector3::ZERO)
	{
	}

	QuadTreeNode::~QuadTreeNode()
	{	
		SAFE_DELETE(mRightTopNode)
		SAFE_DELETE(mLeftTopNode)
		SAFE_DELETE(mRightBottomNode)
		SAFE_DELETE(mLeftBottomNode)
		
		// 不删除 mParentNode，因为父节点的生命周期由上层管理
		mParentNode = nullptr;
		
		mRenderProxys.clear();
	}

	void QuadTreeNode::AddRenderProxy(RenderProxy* InProxy)
	{
		if (InProxy == nullptr)
			return;
			
		// 检查是否已存在
		auto iter = std::find(mRenderProxys.begin(), mRenderProxys.end(), InProxy);
		if (iter != mRenderProxys.end())
			return;
			
		mRenderProxys.push_back(InProxy);
	}

	void QuadTreeNode::RemoveRenderProxy(RenderProxy* InProxy)
	{
		if (InProxy == nullptr)
			return;
			
		auto iter = std::find(mRenderProxys.begin(), mRenderProxys.end(), InProxy);
		if (iter != mRenderProxys.end())
		{
			mRenderProxys.erase(iter);
		}
	}

	bool QuadTreeNode::IsPointInNode(const Vector3& point) const
	{
		const Vector3& min = mNodeBounds.getMin();
		const Vector3& max = mNodeBounds.getMax();
		
		return point.x >= min.x && point.x <= max.x &&
			   point.y >= min.y && point.y <= max.y &&
			   point.z >= min.z && point.z <= max.z;
	}

	bool QuadTreeNode::IsAABBIntersect(const AxisAlignedBox& aabb) const
	{
		const Vector3& nodeMin = mNodeBounds.getMin();
		const Vector3& nodeMax = mNodeBounds.getMax();
		const Vector3& aabbMin = aabb.getMin();
		const Vector3& aabbMax = aabb.getMax();
		
		// AABB碰撞检测：如果两个盒子在任何轴上都不重叠，则不相交
		return !(aabbMax.x < nodeMin.x || aabbMin.x > nodeMax.x ||
				 aabbMax.y < nodeMin.y || aabbMin.y > nodeMax.y ||
				 aabbMax.z < nodeMin.z || aabbMin.z > nodeMax.z);
	}

	QuadTreeNode* QuadTreeNode::GetChild(int index) const
	{
		switch (index)
		{
		case 0: return mRightTopNode;
		case 1: return mLeftTopNode;
		case 2: return mRightBottomNode;
		case 3: return mLeftBottomNode;
		default: return nullptr;
		}
	}

	// ======================== QuadTree Implementation ========================

	QuadTree::QuadTree(const Vector3& InMin, const Vector3& InMax)
	{
		Initialize(InMin, InMax);
	}

	QuadTree::~QuadTree()
	{
		SAFE_DELETE(m_rootNode)
	}

	void QuadTree::Initialize(const Vector3& InMin, const Vector3& InMax)
	{
		m_TreeBounds.setExtent(InMin, InMax);
		m_depth = 0;
		m_maxDepth = MAX_DEPTH;
		m_minNodeSize = MIN_NODE_SIZE;
		
		m_rootNode = new QuadTreeNode();
		m_rootNode->mNodeBounds = m_TreeBounds;
		m_rootNode->mdepth = 0;
		m_rootNode->mParentNode = nullptr;
	}

	QuadTreeNode* QuadTree::CreateNode()
	{
		return new QuadTreeNode();
	}

	void QuadTree::AddRenderProxy(RenderProxy* InProxy)
	{
		if (InProxy == nullptr || m_rootNode == nullptr)
			return;

		// 初始化缓存数据
		if (m_proxyTransformCache.find(InProxy) == m_proxyTransformCache.end())
		{
			ProxyTransformCache cache;
			cache.lastWorldMatrix = InProxy->GetWorldMatrix();
			cache.lastWorldAABB = InProxy->GetWorldAABB();
			cache.lastNode = nullptr;
			m_proxyTransformCache[InProxy] = cache;
		}
		
		InsertRenderProxyRecursive(m_rootNode, InProxy);
	}

	void QuadTree::UpdateRenderProxy(RenderProxy* InProxy)
	{
		if (InProxy == nullptr || m_rootNode == nullptr)
			return;

		// 查找缓存
		auto iter = m_proxyTransformCache.find(InProxy);
		if (iter == m_proxyTransformCache.end())
		{
			// 如果没有缓存，直接添加
			AddRenderProxy(InProxy);
			return;
		}

		ProxyTransformCache& cache = iter->second;
		
		// 检查位置是否改变
		const Matrix4x4& currentMatrix = InProxy->GetWorldMatrix();
		const AxisAlignedBox& currentAABB = InProxy->GetWorldAABB();
		
		// 如果矩阵和AABB都没有改变，不需要更新
		if (cache.lastWorldMatrix == currentMatrix && 
			cache.lastWorldAABB.getMin() == currentAABB.getMin() &&
			cache.lastWorldAABB.getMax() == currentAABB.getMax())
		{
			return;
		}

		// 尝试增量更新：检查proxy是否仍在上次所在的节点中
		if (cache.lastNode != nullptr && cache.lastNode->IsAABBIntersect(currentAABB))
		{
			// 仍在同一节点内，只需更新缓存即可
			cache.lastWorldMatrix = currentMatrix;
			cache.lastWorldAABB = currentAABB;
			return;
		}

		// 否则，需要重新插入：先删除再添加
		RemoveRenderProxy(InProxy);
		AddRenderProxy(InProxy);
	}
	void QuadTree::RemoveRenderProxy(RenderProxy* InProxy)
	{
		if (InProxy == nullptr || m_rootNode == nullptr)
			return;

		RemoveRenderProxyRecursive(m_rootNode, InProxy);
		
		// 删除缓存
		m_proxyTransformCache.erase(InProxy);
	}

	void QuadTree::FindIntersectingObjects(const AxisAlignedBox& queryAABB, std::vector<RenderProxy*>& outProxies)
	{
		if (m_rootNode == nullptr)
			return;
			
		outProxies.clear();
		GetIntersectingObjectsRecursive(m_rootNode, queryAABB, outProxies);
	}

	void QuadTree::FindVisibleObjectsInFrustum(const ViewFrustum& frustum, std::vector<RenderProxy*>& outProxies)
	{
		if (m_rootNode == nullptr)
			return;
			
		outProxies.clear();
		GetVisibleObjectsInFrustumRecursive(m_rootNode, frustum, outProxies);
	}

	void QuadTree::FindVisibleObjectsByCamera(Camera* InCamera, std::vector<RenderProxy*>& outProxies)
	{
		if (InCamera == nullptr)
			return;

		// 更新视锥体缓存
		UpdateFrustum(InCamera);
		
		// 使用缓存的视锥体查询
		FindVisibleObjectsInFrustum(m_cachedFrustum, outProxies);
	}

	void QuadTree::UpdateFrustum(Camera* InCamera)
	{
		if (InCamera == nullptr)
			return;

		m_cachedCamera = InCamera;
		m_cachedFrustum.BuildFromCamera(InCamera->GetViewProjMatrix());
	}

	void QuadTree::FindVisibilityObject()
	{

	}

	void QuadTree::InsertRenderProxyRecursive(QuadTreeNode* node, RenderProxy* proxy)
	{
		if (node == nullptr || proxy == nullptr)
			return;

		// 检查proxy的AABB是否与节点相交
		if (!node->IsAABBIntersect(proxy->GetWorldAABB()))
		{
			return;
		}

		// 如果该节点还没有子节点，直接添加到当前节点
		if (!node->HasChildren())
		{
			// 检查是否需要细分
			Vector3 size = node->mNodeBounds.getSize();
			float minSize = std::min({size.x, size.y, size.z});
			
			if (minSize > m_minNodeSize && node->mdepth < m_maxDepth)
			{
				// 节点大小足够大且未达到最大深度，进行细分
				SubdivideNode(node);
				
				// 细分后，继续尝试插入到子节点
				if (node->HasChildren())
				{
					InsertRenderProxyRecursive(node->mRightTopNode, proxy);
					InsertRenderProxyRecursive(node->mLeftTopNode, proxy);
					InsertRenderProxyRecursive(node->mRightBottomNode, proxy);
					InsertRenderProxyRecursive(node->mLeftBottomNode, proxy);
					return;
				}
			}
			
			// 无法继续细分或不需要细分，添加到当前节点
			node->AddRenderProxy(proxy);
		}
		else
		{
			// 有子节点，尝试添加到子节点
			bool addedToChild = false;
			
			if (node->mRightTopNode->IsAABBIntersect(proxy->GetWorldAABB()))
			{
				InsertRenderProxyRecursive(node->mRightTopNode, proxy);
				addedToChild = true;
			}
			if (node->mLeftTopNode->IsAABBIntersect(proxy->GetWorldAABB()))
			{
				InsertRenderProxyRecursive(node->mLeftTopNode, proxy);
				addedToChild = true;
			}
			if (node->mRightBottomNode->IsAABBIntersect(proxy->GetWorldAABB()))
			{
				InsertRenderProxyRecursive(node->mRightBottomNode, proxy);
				addedToChild = true;
			}
			if (node->mLeftBottomNode->IsAABBIntersect(proxy->GetWorldAABB()))
			{
				InsertRenderProxyRecursive(node->mLeftBottomNode, proxy);
				addedToChild = true;
			}
			
			// 如果proxy跨越多个子节点或不完全在任何一个子节点内，也添加到当前节点
			if (!addedToChild || proxy->GetWorldAABB().getSize().length() > node->mNodeBounds.getSize().length() * 0.4f)
			{
				node->AddRenderProxy(proxy);
			}
		}
	}

	void QuadTree::RemoveRenderProxyRecursive(QuadTreeNode* node, RenderProxy* proxy)
	{
		if (node == nullptr || proxy == nullptr)
			return;

		// 从当前节点删除
		node->RemoveRenderProxy(proxy);

		// 从子节点递归删除
		if (node->HasChildren())
		{
			RemoveRenderProxyRecursive(node->mRightTopNode, proxy);
			RemoveRenderProxyRecursive(node->mLeftTopNode, proxy);
			RemoveRenderProxyRecursive(node->mRightBottomNode, proxy);
			RemoveRenderProxyRecursive(node->mLeftBottomNode, proxy);
			
			// 尝试合并节点（如果所有子节点都为空）
			TryMergeNode(node);
		}
	}

	void QuadTree::GetIntersectingObjectsRecursive(QuadTreeNode* node, const AxisAlignedBox& queryAABB, 
												   std::vector<RenderProxy*>& outProxies)
	{
		if (node == nullptr)
			return;

		// 检查节点是否与查询AABB相交
		if (!node->IsAABBIntersect(queryAABB))
		{
			return;
		}

		// 添加当前节点中与查询AABB相交的所有RenderProxy
		for (RenderProxy* proxy : node->GetRenderProxies())
		{
			if (proxy != nullptr && proxy->IsVisible() && proxy->GetWorldAABB().getMin() != proxy->GetWorldAABB().getMax())
			{
				// 检查proxy的AABB是否与查询AABB相交
				const Vector3& proxyMin = proxy->GetWorldAABB().getMin();
				const Vector3& proxyMax = proxy->GetWorldAABB().getMax();
				const Vector3& queryMin = queryAABB.getMin();
				const Vector3& queryMax = queryAABB.getMax();
				
				if (!(proxyMax.x < queryMin.x || proxyMin.x > queryMax.x ||
					  proxyMax.y < queryMin.y || proxyMin.y > queryMax.y ||
					  proxyMax.z < queryMin.z || proxyMin.z > queryMax.z))
				{
					// 检查是否已存在于结果中
					auto iter = std::find(outProxies.begin(), outProxies.end(), proxy);
					if (iter == outProxies.end())
					{
						outProxies.push_back(proxy);
					}
				}
			}
		}

		// 递归检查子节点
		if (node->HasChildren())
		{
			GetIntersectingObjectsRecursive(node->mRightTopNode, queryAABB, outProxies);
			GetIntersectingObjectsRecursive(node->mLeftTopNode, queryAABB, outProxies);
			GetIntersectingObjectsRecursive(node->mRightBottomNode, queryAABB, outProxies);
			GetIntersectingObjectsRecursive(node->mLeftBottomNode, queryAABB, outProxies);
		}
	}

	void QuadTree::GetVisibleObjectsInFrustumRecursive(QuadTreeNode* node, const ViewFrustum& frustum,
													   std::vector<RenderProxy*>& outProxies)
	{
		if (node == nullptr)
			return;

		// 检查节点是否与视锥体相交
		if (!frustum.IsAABBInFrustum(node->mNodeBounds))
		{
			return;
		}

		// 添加当前节点中所有在视锥体内的RenderProxy
		for (RenderProxy* proxy : node->GetRenderProxies())
		{
			if (proxy != nullptr && proxy->IsVisible() && proxy->GetWorldAABB().getMin() != proxy->GetWorldAABB().getMax())
			{
				// 检查proxy的AABB是否在视锥体内
				if (frustum.IsAABBInFrustum(proxy->GetWorldAABB()))
				{
					// 检查是否已存在于结果中
					auto iter = std::find(outProxies.begin(), outProxies.end(), proxy);
					if (iter == outProxies.end())
					{
						outProxies.push_back(proxy);
					}
				}
			}
		}

		// 递归检查子节点
		if (node->HasChildren())
		{
			GetVisibleObjectsInFrustumRecursive(node->mRightTopNode, frustum, outProxies);
			GetVisibleObjectsInFrustumRecursive(node->mLeftTopNode, frustum, outProxies);
			GetVisibleObjectsInFrustumRecursive(node->mRightBottomNode, frustum, outProxies);
			GetVisibleObjectsInFrustumRecursive(node->mLeftBottomNode, frustum, outProxies);
		}
	}

	void QuadTree::SubdivideNode(QuadTreeNode* node)
	{
		if (node == nullptr || node->HasChildren())
			return;

		Vector3 min = node->mNodeBounds.getMin();
		Vector3 max = node->mNodeBounds.getMax();
		Vector3 center = node->mNodeBounds.getCenter();

		// 创建四个子节点
		// 右上 (Right-Top, 1象限)
		node->mRightTopNode = CreateNode();
		node->mRightTopNode->mNodeBounds.setExtent(
			Vector3(center.x, center.y, min.z),
			Vector3(max.x, max.y, max.z)
		);
		node->mRightTopNode->mdepth = node->mdepth + 1;
		node->mRightTopNode->mParentNode = node;

		// 左上 (Left-Top, 2象限)
		node->mLeftTopNode = CreateNode();
		node->mLeftTopNode->mNodeBounds.setExtent(
			Vector3(min.x, center.y, min.z),
			Vector3(center.x, max.y, max.z)
		);
		node->mLeftTopNode->mdepth = node->mdepth + 1;
		node->mLeftTopNode->mParentNode = node;

		// 右下 (Right-Bottom, 4象限)
		node->mRightBottomNode = CreateNode();
		node->mRightBottomNode->mNodeBounds.setExtent(
			Vector3(center.x, min.y, min.z),
			Vector3(max.x, center.y, max.z)
		);
		node->mRightBottomNode->mdepth = node->mdepth + 1;
		node->mRightBottomNode->mParentNode = node;

		// 左下 (Left-Bottom, 3象限)
		node->mLeftBottomNode = CreateNode();
		node->mLeftBottomNode->mNodeBounds.setExtent(
			Vector3(min.x, min.y, min.z),
			Vector3(center.x, center.y, max.z)
		);
		node->mLeftBottomNode->mdepth = node->mdepth + 1;
		node->mLeftBottomNode->mParentNode = node;

		// 将当前节点的所有RenderProxy重新分配给子节点
		std::vector<RenderProxy*> proxies = node->mRenderProxys;
		node->mRenderProxys.clear();
		
		for (RenderProxy* proxy : proxies)
		{
			if (proxy != nullptr)
			{
				// 尝试将proxy完全放入子节点中
				bool placedInChild = false;
				
				std::vector<QuadTreeNode*> intersectingChildren;
				if (node->mRightTopNode->IsAABBIntersect(proxy->GetWorldAABB()))
					intersectingChildren.push_back(node->mRightTopNode);
				if (node->mLeftTopNode->IsAABBIntersect(proxy->GetWorldAABB()))
					intersectingChildren.push_back(node->mLeftTopNode);
				if (node->mRightBottomNode->IsAABBIntersect(proxy->GetWorldAABB()))
					intersectingChildren.push_back(node->mRightBottomNode);
				if (node->mLeftBottomNode->IsAABBIntersect(proxy->GetWorldAABB()))
					intersectingChildren.push_back(node->mLeftBottomNode);
				
				// 如果proxy只与一个子节点相交，则添加到那个子节点
				if (intersectingChildren.size() == 1)
				{
					intersectingChildren[0]->AddRenderProxy(proxy);
					placedInChild = true;
				}
				else if (intersectingChildren.size() > 1)
				{
					// 如果proxy与多个子节点相交，添加到它们所有相交的节点
					for (QuadTreeNode* child : intersectingChildren)
					{
						child->AddRenderProxy(proxy);
					}
					placedInChild = true;
				}
				
				// 如果没有放入子节点或者proxy很大，保留在父节点中
				if (!placedInChild)
				{
					node->mRenderProxys.push_back(proxy);
				}
			}
		}
	}

	void QuadTree::TryMergeNode(QuadTreeNode* node)
	{
		if (node == nullptr || !node->HasChildren())
			return;

		// 计算所有子节点中的RenderProxy总数
		int totalProxies = 0;
		totalProxies += node->mRightTopNode->GetRenderProxies().size();
		totalProxies += node->mLeftTopNode->GetRenderProxies().size();
		totalProxies += node->mRightBottomNode->GetRenderProxies().size();
		totalProxies += node->mLeftBottomNode->GetRenderProxies().size();

		// 如果子节点中的proxy数量少于某个阈值，可以考虑合并
		// 这里我们设置为 < 4，即如果子节点总共不超过4个proxy，就合并
		if (totalProxies < 4 && 
			!node->mRightTopNode->HasChildren() && 
			!node->mLeftTopNode->HasChildren() && 
			!node->mRightBottomNode->HasChildren() && 
			!node->mLeftBottomNode->HasChildren())
		{
			// 合并：将所有子节点的proxy移到当前节点
			for (RenderProxy* proxy : node->mRightTopNode->GetRenderProxies())
			{
				node->AddRenderProxy(proxy);
			}
			for (RenderProxy* proxy : node->mLeftTopNode->GetRenderProxies())
			{
				node->AddRenderProxy(proxy);
			}
			for (RenderProxy* proxy : node->mRightBottomNode->GetRenderProxies())
			{
				node->AddRenderProxy(proxy);
			}
			for (RenderProxy* proxy : node->mLeftBottomNode->GetRenderProxies())
			{
				node->AddRenderProxy(proxy);
			}

			// 删除子节点
			SAFE_DELETE(node->mRightTopNode)
			SAFE_DELETE(node->mLeftTopNode)
			SAFE_DELETE(node->mRightBottomNode)
			SAFE_DELETE(node->mLeftBottomNode)
		}
	}
}