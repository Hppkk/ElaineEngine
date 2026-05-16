
#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineMaterialInstanceStatic.h"

namespace Elaine
{
    /**
     * MaterialParamSnapshot — 材质参数快照
     * 
     * 由逻辑线程（从 MaterialInstanceDynamic）创建，通过 ENQUEUE_RENDER_COMMAND
     * 传递到渲染线程的 RenderMaterialProxy。
     * 
     * 设计原则：
     * - 仅包含 ResourcePtr（引用计数安全）和 POD 参数值
     * - 不包含任何 RHI 资源指针（RHITexture*, RHIPipeline* 等）
     * - 可安全拷贝/移动跨线程传递
     */
    struct MaterialParamSnapshot
    {
        /// 底层材质实例资源（引用计数安全，指向 MaterialInstanceStatic）
        MaterialInstanceStaticPtr Source;

        /// 覆盖的纹理参数（TexturePtr 是 ResourcePtr<Texture>，引用计数安全）
        std::unordered_map<TextureSemantics, TexturePtr> OverridedTextures;

        /// 覆盖的标量参数（Key 为参数名 hash）
        std::unordered_map<uint64_t, float> OverridedScalars;

        /// 覆盖的向量参数（Key 为参数名 hash）
        std::unordered_map<uint64_t, Vector4> OverridedVectors;

        /// 快照是否有效（Source 非空且已加载）
        bool IsValid() const
        {
            return !Source.isNull();
        }

        /// 检查底层资源链是否全部加载完成
        bool IsFullyLoaded() const
        {
            if (Source.isNull())
                return false;
            if (!Source->IsLoaded())
                return false;
            const MaterialPtr& OriginMat = Source->GetOriginMaterial();
            if (OriginMat.isNull())
                return false;
            return OriginMat->IsLoaded();
        }
    };
}
