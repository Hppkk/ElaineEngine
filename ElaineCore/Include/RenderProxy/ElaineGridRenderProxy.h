#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "RenderProxy/ElaineRenderMaterialProxy.h"

namespace Elaine
{
    class ElaineCoreExport GridRenderProxy : public RenderProxy
    {
    public:
        GridRenderProxy();
        virtual ~GridRenderProxy();

        /**
         * 从逻辑线程的 MaterialParamSnapshot 更新渲染线程的材质代理。
         * 必须在渲染线程（ENQUEUE_RENDER_COMMAND 的 lambda 中）调用。
         */
        void UpdateMaterial(const MaterialParamSnapshot& InSnapshot) { mMaterialProxy.UpdateFromSnapshot(InSnapshot); }

        /** 获取渲染线程材质代理（仅渲染线程访问） */
        RenderMaterialProxy& GetMaterialProxy() { return mMaterialProxy; }
        const RenderMaterialProxy& GetMaterialProxy() const { return mMaterialProxy; }

        // ========== 重写基类虚函数 ==========
        void UpdateRenderQueue(RenderQueueSet* InRenderQueue) override;
        void PrepareResourceBinding() override;
        void InitializeResourceBinding() override;

    private:
        RenderMaterialProxy mMaterialProxy;   // 渲染线程材质代理（值语义）
    };
}