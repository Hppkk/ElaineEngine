#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineResourceLoader.h"
#include "Resource/ElaineResourcePipeline.h"
namespace Elaine
{
    void ResourceLoader::RegisterManager(ResourceType Type, ResourceManager* Manager)
    { if (Type < RT_Count) mManagers[Type] = Manager; }
    ResourceManager* ResourceLoader::GetManager(ResourceType Type) const
    { return Type < RT_Count ? mManagers[Type] : nullptr; }
    ResourceRequestBase ResourceLoader::RequestResource(const std::string& Path, ResourceType Type)
    {
        if (auto* Manager = GetManager(Type)) return Manager->RequestResource(Path);
        auto Data = std::make_shared<ResourceRequestData>();
        Data->Finish({{}, ResourceRequestState::Failed, "Unsupported resource type"});
        return ResourceRequestBase(Data);
    }
    ResourceRequestBase ResourceLoader::RequestResource(const std::string& Path)
    { return RequestResource(Path, InferTypeFromPath(Path)); }
    ResourceLoadResult ResourceLoader::LoadBlocking(const std::string& Path, ResourceType Type)
    { return ResourcePipeline::Get().LoadBlocking(RequestResource(Path, Type)); }
    ResourceType ResourceLoader::InferTypeFromPath(const std::string& Path)
    {
        const auto Dot = Path.rfind('.');
        if (Dot == std::string::npos) return RT_Count;
        std::string Ext = Path.substr(Dot);
        for (auto& C : Ext) C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
        if (Ext == ".texture" || Ext == ".tex") return RT_Texture;
        if (Ext == ".material") return RT_Material;
        if (Ext == ".mi") return RT_MaterialInstance;
        if (Ext == ".vs" || Ext == ".ps" || Ext == ".gs" || Ext == ".cs" || Ext == ".vert" || Ext == ".frag") return RT_Shader;
        if (Ext == ".mesh") return RT_Mesh;
        if (Ext == ".actor") return RT_Actor;
        return RT_Count;
    }
}
