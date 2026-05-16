#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineUniformGPUManager.h"

namespace Elaine
{
    class ShaderPass;
    struct MaterialParamSnapshot;

    enum MaterialType
    {
        MaterialResource,
        MaterialStatic,
        MaterialDynamic
    };

    class ElaineCoreExport MaterialInterface
    {
    public:
        virtual ~MaterialInterface() = default;

        virtual ShaderPass* GetShaderPass() { return nullptr; }
        const MaterialType GetMaterialType() const { return mMaterialType; }

        /**
         * 创建材质参数快照，用于通过 ENQUEUE_RENDER_COMMAND 传递到渲染线程。
         * 仅 MaterialInstanceDynamic 需要实现此接口。
         * 快照中不包含任何 RHI 资源，只包含 ResourcePtr 和 POD 参数。
         * @return 材质参数快照
         */
        virtual MaterialParamSnapshot CreateSnapshot() const;

    protected:
        MaterialType mMaterialType = MaterialResource;
    };
}