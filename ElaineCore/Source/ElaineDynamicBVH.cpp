#include "ElainePrecompiledHeader.h"
#include "math/ElaineDynamicBVH.h"
#include "math/ElaineISpatialObject.h"
#include "math/ElaineRay.h"

namespace Elaine
{
	static AxisAlignedBox MergeAABB(const AxisAlignedBox& a, const AxisAlignedBox& b)
	{
		AxisAlignedBox res = a;
		res.merge(b);
		return res;
	}

	static float AABBSurfaceArea(const AxisAlignedBox& box)
	{
		Vector3 d = box.getMax() - box.getMin();
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	static bool IntersectRayAABB(const Ray& InRay, const AxisAlignedBox& InAABB, float& OutDistance)
	{
		if (InAABB.isNull()) return false;
		if (InAABB.m_extent == AxisAlignedBox::Extent_Infinite) { OutDistance = 0.0f; return true; }

		Vector3 invDir = Vector3(
			InRay.GetDirection().x != 0.0f ? 1.0f / InRay.GetDirection().x : FLT_MAX,
			InRay.GetDirection().y != 0.0f ? 1.0f / InRay.GetDirection().y : FLT_MAX,
			InRay.GetDirection().z != 0.0f ? 1.0f / InRay.GetDirection().z : FLT_MAX
		);

		float t1 = (InAABB.getMin().x - InRay.GetOrigin().x) * invDir.x;
		float t2 = (InAABB.getMax().x - InRay.GetOrigin().x) * invDir.x;
		float tMinX = std::min(t1, t2);
		float tMaxX = std::max(t1, t2);

		t1 = (InAABB.getMin().y - InRay.GetOrigin().y) * invDir.y;
		t2 = (InAABB.getMax().y - InRay.GetOrigin().y) * invDir.y;
		float tMinY = std::min(t1, t2);
		float tMaxY = std::max(t1, t2);

		t1 = (InAABB.getMin().z - InRay.GetOrigin().z) * invDir.z;
		t2 = (InAABB.getMax().z - InRay.GetOrigin().z) * invDir.z;
		float tMinZ = std::min(t1, t2);
		float tMaxZ = std::max(t1, t2);

		OutDistance = std::max(std::max(tMinX, tMinY), tMinZ);
		float tMax = std::min(std::min(tMaxX, tMaxY), tMaxZ);

		if (tMax < 0.0f || OutDistance > tMax) return false;
		if (OutDistance < 0.0f) OutDistance = 0.0f; 
		return true;
	}

    bool AABBIntersect(const AxisAlignedBox& a, const AxisAlignedBox& b)
	{
		if (a.isNull() || b.isNull()) return false;
		if (a.m_extent == AxisAlignedBox::Extent_Infinite || b.m_extent == AxisAlignedBox::Extent_Infinite) return true;

		Vector3 aMin = a.getMin();
		Vector3 aMax = a.getMax();
		Vector3 bMin = b.getMin();
		Vector3 bMax = b.getMax();

		if (aMax.x < bMin.x || aMin.x > bMax.x) return false;
		if (aMax.y < bMin.y || aMin.y > bMax.y) return false;
		if (aMax.z < bMin.z || aMin.z > bMax.z) return false;
		return true;
	}

    DynamicBVH::DynamicBVH()
    {
        mRootIndex = -1;
        mNodeCapacity = 16;
        mNodeCount = 0;
        mNodes.resize(mNodeCapacity);

        for (int32_t i = 0; i < mNodeCapacity - 1; ++i)
        {
            mNodes[i].ParentIndex = i + 1;
            mNodes[i].Height = -1;
        }
        mNodes[mNodeCapacity - 1].ParentIndex = -1;
        mNodes[mNodeCapacity - 1].Height = -1;
        mFreeList = 0;
    }

    DynamicBVH::~DynamicBVH()
    {
    }

    int32_t DynamicBVH::AllocateNode()
    {
        if (mFreeList == -1)
        {
            mNodeCapacity *= 2;
            mNodes.resize(mNodeCapacity);

            for (int32_t i = mNodeCount; i < mNodeCapacity - 1; ++i)
            {
                mNodes[i].ParentIndex = i + 1;
                mNodes[i].Height = -1;
            }
            mNodes[mNodeCapacity - 1].ParentIndex = -1;
            mNodes[mNodeCapacity - 1].Height = -1;
            mFreeList = mNodeCount;
        }

        int32_t NodeId = mFreeList;
        mFreeList = mNodes[NodeId].ParentIndex;

        mNodes[NodeId].ParentIndex = -1;
        mNodes[NodeId].Child1 = -1;
        mNodes[NodeId].Child2 = -1;
        mNodes[NodeId].Height = 0;
        mNodes[NodeId].Object = nullptr;

        ++mNodeCount;
        return NodeId;
    }

    void DynamicBVH::FreeNode(int32_t NodeId)
    {
        mNodes[NodeId].ParentIndex = mFreeList;
        mNodes[NodeId].Height = -1;
        mFreeList = NodeId;
        --mNodeCount;
    }

    int32_t DynamicBVH::InsertObject(ISpatialObject* Object)
    {
        if (!Object) return -1;
        
        int32_t LeafId = AllocateNode();
        mNodes[LeafId].AABB = Object->GetBoundingBox();
        mNodes[LeafId].Object = Object;
        
        InsertLeaf(LeafId);
        Object->SetBVHNodeID(LeafId);

        return LeafId;
    }

    void DynamicBVH::RemoveObject(ISpatialObject* Object)
    {
        if (!Object) return;
        int32_t LeafId = Object->GetBVHNodeID();
        if (LeafId == -1) return;

        RemoveLeaf(LeafId);
        FreeNode(LeafId);
        Object->SetBVHNodeID(-1);
    }

    void DynamicBVH::UpdateObject(ISpatialObject* Object)
    {
        // Simple un-optimized update: remove and re-insert
        // A robust BVH would check if the new AABB fits in a fatted AABB before reinserting.
        RemoveObject(Object);
        InsertObject(Object);
    }

    void DynamicBVH::Clear()
    {
        mRootIndex = -1;
        mNodeCount = 0;
        mFreeList = 0;
        for (int32_t i = 0; i < mNodeCapacity - 1; ++i)
        {
            mNodes[i].ParentIndex = i + 1;
            mNodes[i].Height = -1;
        }
        mNodes[mNodeCapacity - 1].ParentIndex = -1;
        mNodes[mNodeCapacity - 1].Height = -1;
    }

    void DynamicBVH::InsertLeaf(int32_t LeafId)
    {
        if (mRootIndex == -1)
        {
            mRootIndex = LeafId;
            mNodes[mRootIndex].ParentIndex = -1;
            return;
        }

        AxisAlignedBox LeafAABB = mNodes[LeafId].AABB;
        int32_t Index = mRootIndex;

        // Find the best sibling
        while (!mNodes[Index].IsLeaf())
        {
            int32_t Child1 = mNodes[Index].Child1;
            int32_t Child2 = mNodes[Index].Child2;

            float Area = AABBSurfaceArea(mNodes[Index].AABB);

            AxisAlignedBox CombinedAABB = MergeAABB(mNodes[Index].AABB, LeafAABB);
            float CombinedArea = AABBSurfaceArea(CombinedAABB);
            float Cost = 2.0f * CombinedArea;
            float InheritanceCost = 2.0f * (CombinedArea - Area);

            float Cost1;
            {
                AxisAlignedBox aabb = MergeAABB(LeafAABB, mNodes[Child1].AABB);
                float newArea = AABBSurfaceArea(aabb);
                float oldArea = AABBSurfaceArea(mNodes[Child1].AABB);
                Cost1 = (newArea - oldArea) + InheritanceCost;
                if (mNodes[Child1].IsLeaf()) Cost1 = newArea + InheritanceCost;
            }

            float Cost2;
            {
                AxisAlignedBox aabb = MergeAABB(LeafAABB, mNodes[Child2].AABB);
                float newArea = AABBSurfaceArea(aabb);
                float oldArea = AABBSurfaceArea(mNodes[Child2].AABB);
                Cost2 = (newArea - oldArea) + InheritanceCost;
                if (mNodes[Child2].IsLeaf()) Cost2 = newArea + InheritanceCost;
            }

            if (Cost < Cost1 && Cost < Cost2)
                break;

            Index = Cost1 < Cost2 ? Child1 : Child2;
        }

        int32_t Sibling = Index;
        int32_t OldParent = mNodes[Sibling].ParentIndex;
        int32_t NewParent = AllocateNode();

        mNodes[NewParent].ParentIndex = OldParent;
        mNodes[NewParent].AABB = MergeAABB(LeafAABB, mNodes[Sibling].AABB);
        mNodes[NewParent].Height = std::max(mNodes[LeafId].Height, mNodes[Sibling].Height) + 1;

        if (OldParent != -1)
        {
            if (mNodes[OldParent].Child1 == Sibling) mNodes[OldParent].Child1 = NewParent;
            else mNodes[OldParent].Child2 = NewParent;
        }
        else
        {
            mRootIndex = NewParent;
        }

        mNodes[NewParent].Child1 = Sibling;
        mNodes[NewParent].Child2 = LeafId;
        mNodes[Sibling].ParentIndex = NewParent;
        mNodes[LeafId].ParentIndex = NewParent;

        SyncHierarchy(mNodes[LeafId].ParentIndex);
    }

    void DynamicBVH::RemoveLeaf(int32_t LeafId)
    {
        if (LeafId == mRootIndex)
        {
            mRootIndex = -1;
            return;
        }

        int32_t Parent = mNodes[LeafId].ParentIndex;
        int32_t GrandParent = mNodes[Parent].ParentIndex;
        int32_t Sibling;

        if (mNodes[Parent].Child1 == LeafId) Sibling = mNodes[Parent].Child2;
        else Sibling = mNodes[Parent].Child1;

        if (GrandParent != -1)
        {
            if (mNodes[GrandParent].Child1 == Parent) mNodes[GrandParent].Child1 = Sibling;
            else mNodes[GrandParent].Child2 = Sibling;

            mNodes[Sibling].ParentIndex = GrandParent;
            FreeNode(Parent);

            SyncHierarchy(GrandParent);
        }
        else
        {
            mRootIndex = Sibling;
            mNodes[Sibling].ParentIndex = -1;
            FreeNode(Parent);
        }
    }

    void DynamicBVH::SyncHierarchy(int32_t Index)
    {
        while (Index != -1)
        {
            Index = Balance(Index);

            int32_t Child1 = mNodes[Index].Child1;
            int32_t Child2 = mNodes[Index].Child2;

            mNodes[Index].Height = 1 + std::max(mNodes[Child1].Height, mNodes[Child2].Height);
            mNodes[Index].AABB = MergeAABB(mNodes[Child1].AABB, mNodes[Child2].AABB);

            Index = mNodes[Index].ParentIndex;
        }
    }

    int32_t DynamicBVH::Balance(int32_t iA)
    {
        if (mNodes[iA].IsLeaf() || mNodes[iA].Height < 2) return iA;

        int32_t iB = mNodes[iA].Child1;
        int32_t iC = mNodes[iA].Child2;

        int32_t BalanceVal = mNodes[iC].Height - mNodes[iB].Height;

        if (BalanceVal > 1)
        {
            int32_t iF = mNodes[iC].Child1;
            int32_t iG = mNodes[iC].Child2;

            mNodes[iC].Child1 = iA;
            mNodes[iC].ParentIndex = mNodes[iA].ParentIndex;
            mNodes[iA].ParentIndex = iC;

            if (mNodes[iC].ParentIndex != -1)
            {
                if (mNodes[mNodes[iC].ParentIndex].Child1 == iA) mNodes[mNodes[iC].ParentIndex].Child1 = iC;
                else mNodes[mNodes[iC].ParentIndex].Child2 = iC;
            }
            else
            {
                mRootIndex = iC;
            }

            if (mNodes[iF].Height > mNodes[iG].Height)
            {
                mNodes[iC].Child2 = iF;
                mNodes[iA].Child2 = iG;
                mNodes[iG].ParentIndex = iA;
                mNodes[iA].AABB = MergeAABB(mNodes[iB].AABB, mNodes[iG].AABB);
                mNodes[iC].AABB = MergeAABB(mNodes[iA].AABB, mNodes[iF].AABB);

                mNodes[iA].Height = 1 + std::max(mNodes[iB].Height, mNodes[iG].Height);
                mNodes[iC].Height = 1 + std::max(mNodes[iA].Height, mNodes[iF].Height);
            }
            else
            {
                mNodes[iC].Child2 = iG;
                mNodes[iA].Child2 = iF;
                mNodes[iF].ParentIndex = iA;
                mNodes[iA].AABB = MergeAABB(mNodes[iB].AABB, mNodes[iF].AABB);
                mNodes[iC].AABB = MergeAABB(mNodes[iA].AABB, mNodes[iG].AABB);

                mNodes[iA].Height = 1 + std::max(mNodes[iB].Height, mNodes[iF].Height);
                mNodes[iC].Height = 1 + std::max(mNodes[iA].Height, mNodes[iG].Height);
            }
            return iC;
        }

        if (BalanceVal < -1)
        {
            int32_t iD = mNodes[iB].Child1;
            int32_t iE = mNodes[iB].Child2;

            mNodes[iB].Child1 = iA;
            mNodes[iB].ParentIndex = mNodes[iA].ParentIndex;
            mNodes[iA].ParentIndex = iB;

            if (mNodes[iB].ParentIndex != -1)
            {
                if (mNodes[mNodes[iB].ParentIndex].Child1 == iA) mNodes[mNodes[iB].ParentIndex].Child1 = iB;
                else mNodes[mNodes[iB].ParentIndex].Child2 = iB;
            }
            else
            {
                mRootIndex = iB;
            }

            if (mNodes[iD].Height > mNodes[iE].Height)
            {
                mNodes[iB].Child2 = iD;
                mNodes[iA].Child1 = iE;
                mNodes[iE].ParentIndex = iA;
                mNodes[iA].AABB = MergeAABB(mNodes[iC].AABB, mNodes[iE].AABB);
                mNodes[iB].AABB = MergeAABB(mNodes[iA].AABB, mNodes[iD].AABB);

                mNodes[iA].Height = 1 + std::max(mNodes[iC].Height, mNodes[iE].Height);
                mNodes[iB].Height = 1 + std::max(mNodes[iA].Height, mNodes[iD].Height);
            }
            else
            {
                mNodes[iB].Child2 = iE;
                mNodes[iA].Child1 = iD;
                mNodes[iD].ParentIndex = iA;
                mNodes[iA].AABB = MergeAABB(mNodes[iC].AABB, mNodes[iD].AABB);
                mNodes[iB].AABB = MergeAABB(mNodes[iA].AABB, mNodes[iE].AABB);

                mNodes[iA].Height = 1 + std::max(mNodes[iC].Height, mNodes[iD].Height);
                mNodes[iB].Height = 1 + std::max(mNodes[iA].Height, mNodes[iE].Height);
            }
            return iB;
        }

        return iA;
    }

    DynamicBVH::RaycastResult DynamicBVH::Raycast(const Ray& InRay, float MaxDistance) const
    {
        RaycastResult BestResult;
        BestResult.Distance = MaxDistance;

        if (mRootIndex == -1) return BestResult;

        std::vector<int32_t> Stack;
        Stack.push_back(mRootIndex);

        while (!Stack.empty())
        {
            int32_t NodeId = Stack.back();
            Stack.pop_back();

            const BVHNode& Node = mNodes[NodeId];

            float dist;
            if (IntersectRayAABB(InRay, Node.AABB, dist))
            {
                if (dist > BestResult.Distance) continue;

                if (Node.IsLeaf())
                {
                    if (dist < BestResult.Distance)
                    {
                        BestResult.Distance = dist;
                        BestResult.Object = Node.Object;
                    }
                }
                else
                {
                    Stack.push_back(Node.Child1);
                    Stack.push_back(Node.Child2);
                }
            }
        }

        return BestResult;
    }

    std::vector<ISpatialObject*> DynamicBVH::BoxIntersect(const AxisAlignedBox& Box) const
    {
        std::vector<ISpatialObject*> Results;
        if (Box.isNull() || mRootIndex == -1) return Results;

        std::vector<int32_t> Stack;
        Stack.push_back(mRootIndex);

        while (!Stack.empty())
        {
            int32_t NodeId = Stack.back();
            Stack.pop_back();

            const BVHNode& Node = mNodes[NodeId];

            if (AABBIntersect(Node.AABB, Box))
            {
                if (Node.IsLeaf())
                {
                    Results.push_back(Node.Object);
                }
                else
                {
                    Stack.push_back(Node.Child1);
                    Stack.push_back(Node.Child2);
                }
            }
        }
        return Results;
    }
}
