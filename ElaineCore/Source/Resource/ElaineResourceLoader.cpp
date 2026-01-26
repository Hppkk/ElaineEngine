#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineResourceLoader.h"
#include "Resource/ElaineResourceManager.h"
#include "TaskGraph/ElaineTaskGraph.h"

namespace Elaine
{
	ResourceLoader::ResourceLoader()
	{
	}

	ResourceLoader::~ResourceLoader()
	{
	}

	void ResourceLoader::RegisterManager(ResourceType InType, ResourceManager* InManager)
	{
		if (InType < RT_Count)
		{
			mManagers[InType] = InManager;
		}
	}

	ResourceBasePtr ResourceLoader::LoadResource(const std::string& InPath, ResourceType InType, bool InAsync)
	{
		ResourceManager* Manager = GetManager(InType);
		if (Manager == nullptr)
		{
			return nullptr;
		}

		return Manager->GetResource(InPath, InAsync);
	}

	ResourceBasePtr ResourceLoader::LoadResourceAuto(const std::string& InPath, bool InAsync)
	{
		ResourceType Type = InferTypeFromPath(InPath);
		return LoadResource(InPath, Type, InAsync);
	}

	void ResourceLoader::LoadDependencies(
		const std::vector<std::pair<std::string, ResourceType>>& InDependencies,
		std::vector<std::shared_ptr<TaskGraph::GraphTask>>& OutTasks)
	{
		for (const auto& [Path, Type] : InDependencies)
		{
			ResourceBasePtr Res = LoadResource(Path, Type, true);  // 异步加载
			if (!Res.isNull())
			{
				auto Task = Res->GetLoadTask();
				if (Task)
				{
					OutTasks.push_back(Task);
				}
			}
		}
	}

	ResourceType ResourceLoader::InferTypeFromPath(const std::string& InPath)
	{
		// 获取扩展名
		size_t DotPos = InPath.rfind('.');
		if (DotPos == std::string::npos)
			return RT_Count;  // 无效类型

		std::string Ext = InPath.substr(DotPos);

		// 转小写
		for (auto& c : Ext) c = tolower(c);

		// 根据扩展名判断类型
		if (Ext == ".png" || Ext == ".jpg" || Ext == ".tga" || Ext == ".dds" || Ext == ".hdr" || Ext == ".tex")
			return RT_Texture;
		else if (Ext == ".material")
			return RT_Material;
		else if (Ext == ".mi")
			return RT_MaterialInstance;
		else if (Ext == ".vert" || Ext == ".frag" || Ext == ".glsl" || Ext == ".hlsl" || Ext == ".spv")
			return RT_Shader;
		else if (Ext == ".obj" || Ext == ".fbx" || Ext == ".gltf" || Ext == ".mesh")
			return RT_Mesh;
		else if (Ext == ".prefab" || Ext == ".go")
			return RT_GameObject;

		return RT_Count;  // 未知类型
	}

	ResourceManager* ResourceLoader::GetManager(ResourceType InType) const
	{
		if (InType < RT_Count)
		{
			return mManagers[InType];
		}
		return nullptr;
	}
}
