#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineAxisAlignedBox.h"
#include "ElaineQuaternion.h"
#include "render/common/ElaineRHIProtocol.h"
#include "Resource/ElaineResourceBase.h"
#include <unordered_set>

namespace TaskGraph { class GraphTask; }

namespace Elaine
{
    enum class EProxyType
    {
        StaticMesh,
        Sky,
        Grid,
        Light,
        Particle,
        RVTVolume,
        Unknown
    };

    class RenderQueueSet;

    class ElaineCoreExport RenderProxy
    {
    public:
		// For SceneQuery identification
		void* mUserData = nullptr;
		uint32_t mUserType = 0;
        RenderProxy();
        virtual ~RenderProxy() = default;

        EProxyType GetType() const { return mType; }
        const AxisAlignedBox& GetWorldAABB() const { return mWorldAABB; }
        void SetWorldMatrix(const Matrix4x4& InMat) { mWorldMatrix = InMat; }
        const Matrix4x4& GetWorldMatrix() const { return mWorldMatrix; }
        bool IsVisible() const { return mbVisible; }
        void SetVisible(bool bInVisible) { mbVisible = bInVisible; }
        const Vector3& GetWorldPosition() const { return mWorldPosition; }
        const Vector3& GetWorldScale() const { return mWorldScale; }
        const Quaternion& GetWorldRotation() const { return mWorldRotation; }
        void SetWorldPosition(const Vector3& InPosition) { mWorldPosition = InPosition; }
        void SetWorldScale(const Vector3& InScale) { mWorldScale = InScale; }
        void SetWorldRotation(const Quaternion& InRotation) { mWorldRotation = InRotation; }

        // ========== 渲染队列更新 ==========
        virtual void UpdateRenderQueue(RenderQueueSet* InRenderQueue) = 0;

        // ========== 资源依赖管理（引用计数 + 完成回调） ==========
        /**
         * 追踪资源及其所有递归依赖
         * 当资源加载完成时，会自动追踪其子依赖
         * 当所有资源加载完成后自动触发 InitializeResourceBinding
         * @param InResource - 要追踪的资源
         */
        void TrackResource(ResourceBasePtr InResource);

        /**
         * 开始初始化流程
         * 在所有 TrackResource 调用完成后调用此方法
         * 如果所有资源已加载完成，立即触发初始化
         */
        void BeginInitialization();

        /**
         * 检查是否正在等待资源加载
         */
        bool IsWaitingForResources() const { return mPendingResourceCount.load() > 0; }

        /**
         * 检查资源绑定是否已初始化
         */
        bool IsBindingsInitialized() const { return mBindingsInitialized.load(); }

        // ========== 资源绑定接口 ==========
        virtual RHI_DRAW_RESOURCE_BINDING& GetResourceBinding() { return mResourceBinding; }
        virtual const RHI_DRAW_RESOURCE_BINDING& GetResourceBinding() const { return mResourceBinding; }
        virtual void PrepareResourceBinding() {}

    protected:
        // 子类重写：初始化静态资源绑定
        virtual void InitializeResourceBinding() {}
        virtual void OnResourceInitialized() {}
        virtual void OnResourceFailed() {}

        // 资源追踪内部方法
        void TrackResourceRecursive(ResourceBasePtr InResource);
        void OnResourceLoaded(ResourceBase* InResource);
        void TryInitializeBindings();

    protected:
        Matrix4x4 mWorldMatrix;
        AxisAlignedBox mWorldAABB;
        Quaternion mWorldRotation;
        Vector3 mWorldPosition;
        Vector3 mWorldScale;
        EProxyType mType = EProxyType::Unknown;
        bool mbVisible = true;

        // 资源绑定
        RHI_DRAW_RESOURCE_BINDING mResourceBinding;

        // 资源依赖追踪
        std::unordered_set<ResourceBase*> mTrackedResources;  // 防止重复追踪
        std::atomic<int32_t> mPendingResourceCount{0};        // 待完成资源计数
        std::atomic<bool> mInitializationStarted{false};      // 是否已开始初始化流程
        std::atomic<bool> mBindingsInitialized{false};        // 资源绑定是否已初始化
        std::mutex mTrackMutex;                               // 资源追踪锁
    };
}
