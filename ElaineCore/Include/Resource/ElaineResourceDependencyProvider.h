#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
    class ResourceBase;
    template<typename T> class ResourcePtr;
    using ResourceBasePtr = ResourcePtr<ResourceBase>;

    /**
     * 资源依赖提供者接口
     * 允许非 ResourceBase 类（如 ShaderPass）声明其资源依赖
     * 
     * 使用场景：
     * - ShaderPass 需要声明其依赖的 Shader 资源
     * - 其他非资源类需要参与资源依赖追踪
     */
    class ElaineCoreExport IResourceDependencyProvider
    {
    public:
        virtual ~IResourceDependencyProvider() = default;
        
        /**
         * 获取所有依赖的资源
         * @param OutResources - 输出依赖资源列表
         */
        virtual void GetDependentResources(std::vector<ResourceBasePtr>& OutResources) const = 0;
        
        /**
         * 检查所有依赖是否已加载完成
         */
        virtual bool AreDependenciesReady() const;
    };
}
