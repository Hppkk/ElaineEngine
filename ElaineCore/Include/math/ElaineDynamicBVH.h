#pragma once
#include "ElaineCorePrerequirements.h"
#include "math/ElaineAxisAlignedBox.h"
#include "math/ElaineRay.h"
#include "math/ElaineVector2.h"
#include <vector>

namespace Elaine
{
    class ISpatialObject;

    struct BVHNode
    {
        AxisAlignedBox AABB;
        ISpatialObject* Object = nullptr;
        int32_t ParentIndex = -1;
        int32_t Child1 = -1;
        int32_t Child2 = -1;
        
        // Height of the subtree (0 for leaf)
        int32_t Height = -1;

        bool IsLeaf() const { return Child1 == -1; }
    };

    class ElaineCoreExport DynamicBVH
    {
    public:
        DynamicBVH();
        ~DynamicBVH();

        // 插入对象，返回节点ID
        int32_t InsertObject(ISpatialObject* Object);
        
        // 移除对象
        void RemoveObject(ISpatialObject* Object);
        
        // 更新对象位置（通常是先移除再插入的优化，这里先提供简单实现）
        void UpdateObject(ISpatialObject* Object);

        // 清空树
        void Clear();

        // 查询
        struct RaycastResult
        {
            ISpatialObject* Object = nullptr;
            float Distance = FLT_MAX;
        };
        RaycastResult Raycast(const Ray& InRay, float MaxDistance = FLT_MAX) const;

        // 视锥体/AABB 查询（简单查询相交的对象）
        std::vector<ISpatialObject*> BoxIntersect(const AxisAlignedBox& Box) const;

    private:
        int32_t AllocateNode();
        void FreeNode(int32_t NodeId);
        
        void InsertLeaf(int32_t LeafId);
        void RemoveLeaf(int32_t LeafId);

        int32_t Balance(int32_t iA);
        void SyncHierarchy(int32_t NodeId);

    private:
        int32_t mRootIndex = -1;

        std::vector<BVHNode> mNodes;
        int32_t mNodeCount = 0;
        int32_t mNodeCapacity = 0;

        int32_t mFreeList = -1;
    };
}
