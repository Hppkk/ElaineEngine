#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "Resource/ElaineResourceManager.h"

namespace TaskGraph { class GraphTask; }

namespace Elaine
{
	class ResourceManager;

	/**
	 * 统一资源加载器
	 * - 根据ResourceType路由到对应的资源管理器
	 * - 提供统一的异步/同步加载接口
	 * - 支持依赖链加载
	 */
	class ElaineCoreExport ResourceLoader : public Singleton<ResourceLoader>
	{
	public:
		ResourceLoader();
		~ResourceLoader();

		// ========== 初始化 ==========
		/**
		 * 注册资源管理器
		 * @param InType - 资源类型
		 * @param InManager - 对应的管理器指针
		 */
		void RegisterManager(ResourceType InType, ResourceManager* InManager);

		// ========== 资源加载接口 ==========
		/**
		 * 根据路径和类型加载资源
		 * @param InPath - 资源路径
		 * @param InType - 资源类型
		 * @param InAsync - 是否异步加载
		 * @return 资源指针
		 */
		ResourceBasePtr LoadResource(const std::string& InPath, ResourceType InType, bool InAsync = true);

		/**
		 * 根据路径自动推断类型加载资源
		 * @param InPath - 资源路径 (通过扩展名推断类型)
		 * @param InAsync - 是否异步加载
		 * @return 资源指针
		 */
		ResourceBasePtr LoadResourceAuto(const std::string& InPath, bool InAsync = true);

		/**
		 * 加载依赖列表中的所有资源
		 * @param InDependencies - 依赖信息列表 (路径, 类型)
		 * @param OutTasks - 输出加载任务列表
		 */
		void LoadDependencies(
			const std::vector<std::pair<std::string, ResourceType>>& InDependencies,
			std::vector<std::shared_ptr<TaskGraph::GraphTask>>& OutTasks
		);

		static ResourceType InferTypeFromPath(const std::string& InPath);

		ResourceManager* GetManager(ResourceType InType) const;

	private:
		ResourceManager* mManagers[RT_Count] = { nullptr };
	};
}
