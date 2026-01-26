#include "ElainePrecompiledHeader.h"
#include "ElaineMaterial.h"
#include "ElaineShaderManager.h"
#include "ElaineShaderPassManager.h"
#include "ElaineTextureManager.h"
#include "ElaineDataStream.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "Resource/ElaineResourceManager.h"


namespace Elaine
{
	Material::Material(ResourceManager* InManager, const std::string& InResourceName)
		: ResourceBase(InManager, InResourceName)
	{
		
	}

	Material::~Material()
	{

	}

	void Material::SetTexture(TextureSemantics InSemantics, TexturePtr InTexture)
	{
		mTextures[InSemantics] = InTexture;
		mbDirty = true;
	}

	TexturePtr Material::GetTexture(TextureSemantics InSemantics) const
	{
		return mTextures[InSemantics];
	}

	bool Material::LoadImpl()
	{
		DataStream Stream(Root::instance()->GetResourcePath() + mResourceName);
		Stream.ReadAll();
		JsonCpp JsonMaterial = JsonCpp::parse(Stream.GetDataStream());
		mVersion = Version(JsonMaterial["version"]);

		//ClearDependencies();

		JsonCpp TextureListJson = JsonMaterial["textures"];
		if (!TextureListJson.empty())
		{
			for (auto&& tex : TextureListJson.items())
			{
				std::string TexPath = tex.value();
				
				TextureSemantics TS = SemanticsRegister::GetSemantics(tex.key().c_str());
				mTextures[TS] = TextureManager::instance()->GetResource(TexPath);
				ResourceBasePtr BasePtr = mTextures[TS];
				//AddResourceEvent(ResourceEvent(TexPath.c_str(), RT_Texture, BasePtr->GetLoadTask()));
			}
		}

		// 解析ShaderPass (内部会加载Shader)
		JsonCpp ShaderPassJson = JsonMaterial["shaderpass"];
		if (!ShaderPassJson.empty())
		{
			for (auto&& passJson : ShaderPassJson)
			{
				ShaderPass* NewShaderPass = ShaderPassManager::instance()->CreateShaderPass(passJson);
				mPassMap.emplace(NewShaderPass->GetPassName(), NewShaderPass);
				//AddResourceEvent(NewShaderPass->GetResourceEvents());
			}
		}

		return true;
	}

	void Material::UnloadImpl()
	{

	}

	void Material::SaveResourceImpl()
	{

	}

	void Material::ResourceArrivedImpl()
	{
		//Load Texture
		//for (auto Tex : mTextureRegisters)
		//{
			//TaskGraph::GraphTaskDesc taskDesc;
			//struct ResourceHolder
			//{
			//	ResourceHolder(TexturePtr InPtr)
			//	{
			//		mPtr = InPtr;
			//	}
			//	TexturePtr mPtr;
			//};
			//std::shared_ptr<ResourceHolder> resourceHolder = std::make_shared<ResourceHolder>(mTextures[Tex]);
			//taskDesc.mTaskFunction = [resourceHolder] { resourceHolder->mPtr->LoadResource(); };
			//taskDesc.SubsequentTask([resourceHolder] { resourceHolder->mPtr->ResourceArrived(); });
			//TaskGraph::GraphTaskCreateDesc createDesc;
			//createDesc.mDirectTasks.push_back(taskDesc);
			//TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady(createDesc);

			//TextureManager::instance()->GetResource(mTextures[Tex]->GetPath());
		//}

		//Load Shader

		//auto LoadShaderFunction = [](ShaderPtr InShader)
		//	{
		//		TaskGraph::GraphTaskDesc taskDesc;
		//		struct ResourceHolder
		//		{
		//			ResourceHolder(ShaderPtr InPtr)
		//			{
		//				mPtr = InPtr;
		//			}
		//			ShaderPtr mPtr;
		//		};
		//		std::shared_ptr<ResourceHolder> resourceHolder = std::make_shared<ResourceHolder>(InShader);
		//		taskDesc.mTaskFunction = [resourceHolder] { resourceHolder->mPtr->LoadResource(); };
		//		taskDesc.SubsequentTask([resourceHolder] { resourceHolder->mPtr->ResourceArrived(); });
		//		TaskGraph::GraphTaskCreateDesc createDesc;
		//		createDesc.mDirectTasks.push_back(taskDesc);
		//	};
		//LoadShaderFunction(mVertexShader);
		//LoadShaderFunction(mFragmentShader);

		//ShaderManager::instance()->GetResource(mVertexShader->GetPath());
		//ShaderManager::instance()->GetResource(mFragmentShader->GetPath());
	}

	ShaderPass* Material::GetPass(const Name& InPassType)
	{
		return mPassMap[InPassType];
	}

	bool Material::IsRegisterPass(const Name& InPassType)
	{
		return mPassMap[InPassType] != nullptr;
	}

	void Material::GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const
	{
		// 纹理依赖
		for (int i = 0; i < Tex_Count; ++i)
		{
			if (!mTextures[i].isNull())
			{
				OutDependencies.push_back(mTextures[i]);
			}
		}

		// ShaderPass 中的 Shader 依赖
		for (const auto& [PassName, Pass] : mPassMap)
		{
			if (Pass)
			{
				Pass->GetDependentResources(OutDependencies);
			}
		}
	}
}