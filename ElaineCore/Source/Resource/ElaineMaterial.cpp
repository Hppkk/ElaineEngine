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

	float Material::GetScalar(uint64_t InParamHash, float InDefault) const
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mScalarDefaults.find(InParamHash);
		return It == mScalarDefaults.end() ? InDefault : It->second;
	}

	Vector4 Material::GetVector(uint64_t InParamHash, const Vector4& InDefault) const
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mVectorDefaults.find(InParamHash);
		return It == mVectorDefaults.end() ? InDefault : It->second;
	}

	int32_t Material::GetInt(uint64_t InParamHash, int32_t InDefault) const
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mIntDefaults.find(InParamHash);
		return It == mIntDefaults.end() ? InDefault : It->second;
	}

	bool Material::GetBool(uint64_t InParamHash, bool InDefault) const
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mBoolDefaults.find(InParamHash);
		return It == mBoolDefaults.end() ? InDefault : It->second;
	}

	bool Material::HasScalar(uint64_t InParamHash) const { std::lock_guard<std::mutex> Lock(mDataMutex); return mScalarDefaults.find(InParamHash) != mScalarDefaults.end(); }
	bool Material::HasVector(uint64_t InParamHash) const { std::lock_guard<std::mutex> Lock(mDataMutex); return mVectorDefaults.find(InParamHash) != mVectorDefaults.end(); }
	std::vector<MaterialParameterDesc> Material::GetParameterLayoutSnapshot() const { std::lock_guard<std::mutex> Lock(mDataMutex); return mParameterLayout; }

	Material::~Material()
	{

	}

	void Material::SetTexture(TextureSemantics InSemantics, TexturePtr InTexture)
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		mTextures[InSemantics] = InTexture;
		mbDirty = true;
	}

	TexturePtr Material::GetTexture(TextureSemantics InSemantics) const
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		return mTextures[InSemantics];
	}

	bool Material::LoadImpl()
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		DataStream Stream(Root::instance()->GetResourcePath() + mResourceName);
		Stream.ReadAll();
		JsonCpp JsonMaterial = JsonCpp::parse(Stream.GetDataStream());
		if (!JsonMaterial.contains("version") || JsonMaterial["version"] != "2.0.0")
		{
			LOG_ERROR("Material '%s' requires format version 2.0.0.", mResourceName.c_str());
			return false;
		}
		mVersion = Version(JsonMaterial["version"]);
		mScalarDefaults.clear();
		mVectorDefaults.clear();
		mIntDefaults.clear();
		mBoolDefaults.clear();
		mParameterLayout.clear();
		for (auto& [PassName, Pass] : mPassMap)
			if (Pass) ShaderPassManager::instance()->DestroyShaderPass(Pass);
		mPassMap.clear();
		for (auto& Texture : mTextures)
			Texture = TexturePtr();

		const auto& Parameters = JsonMaterial["parameters"];
		if (Parameters.is_object())
		{
			for (auto& It : Parameters.items())
			{
				const uint64_t Hash = Name(It.key().c_str()).GetHash();
				const auto& Value = It.value();
				const std::string Type = Value.value("type", "");
				const auto& Default = Value.contains("default") ? Value["default"] : JsonCpp();
				MaterialParameterDesc Desc;
				Desc.Name = It.key();
				Desc.Hash = Hash;
				if (Type == "scalar") Desc.Kind = MaterialParameterKind::Scalar;
				else if (Type == "vector") Desc.Kind = MaterialParameterKind::Vector;
				else if (Type == "color") Desc.Kind = MaterialParameterKind::Color;
				else if (Type == "int") Desc.Kind = MaterialParameterKind::Integer;
				else if (Type == "bool") Desc.Kind = MaterialParameterKind::Boolean;
				else if (Type == "texture") Desc.Kind = MaterialParameterKind::Texture;
				if (Value.contains("min") && Value.contains("max")) { Desc.Min = Value["min"].get<float>(); Desc.Max = Value["max"].get<float>(); Desc.HasRange = true; }
				if (Type == "scalar" && Default.is_number())
					mScalarDefaults[Hash] = Default.get<float>();
				else if ((Type == "vector" || Type == "color") && Default.is_array())
					mVectorDefaults[Hash] = Vector4(Default.size() > 0 ? Default[0].get<float>() : 0.0f,
						Default.size() > 1 ? Default[1].get<float>() : 0.0f,
						Default.size() > 2 ? Default[2].get<float>() : 0.0f,
						Default.size() > 3 ? Default[3].get<float>() : 0.0f);
				else if (Type == "int" && Default.is_number_integer())
					mIntDefaults[Hash] = Default.get<int32_t>();
				else if (Type == "bool" && Default.is_boolean())
					mBoolDefaults[Hash] = Default.get<bool>();
				if (Default.is_number()) Desc.DefaultValue = Vector4(Default.get<float>(), 0, 0, 0);
				else if (Default.is_array()) Desc.DefaultValue = Vector4(Default.size() > 0 ? Default[0].get<float>() : 0.0f, Default.size() > 1 ? Default[1].get<float>() : 0.0f, Default.size() > 2 ? Default[2].get<float>() : 0.0f, Default.size() > 3 ? Default[3].get<float>() : 0.0f);
				Desc.HasDefault = !Default.is_null();
				mParameterLayout.push_back(std::move(Desc));
			}
		}

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
				if (NewShaderPass != nullptr)
				{
					mPassMap.emplace(NewShaderPass->GetPassName(), NewShaderPass);
					for (const auto& Reflected : NewShaderPass->GetMaterialParameters())
					{
						uint64 ReflectedHash = Reflected.NameHash;
						std::string ParameterName = Reflected.Name;
						if (ParameterName.rfind("U_", 0) == 0)
						{
							ParameterName = ParameterName.substr(2);
							ReflectedHash = Name(ParameterName.c_str()).GetHash();
						}
						auto It = std::find_if(mParameterLayout.begin(), mParameterLayout.end(), [ReflectedHash](const MaterialParameterDesc& Item) { return Item.Hash == ReflectedHash; });
						if (It == mParameterLayout.end())
						{
							MaterialParameterDesc Desc;
							Desc.Name = ParameterName;
							Desc.Hash = ReflectedHash;
							mParameterLayout.push_back(std::move(Desc));
							It = std::prev(mParameterLayout.end());
						}
						It->Offset = Reflected.Offset;
						It->Size = Reflected.Size;
						It->ArrayCount = Reflected.ArrayCount;
						It->Set = Reflected.Set;
						It->Binding = Reflected.Binding;
						It->Reflected = true;
						switch (Reflected.Type)
						{
						case RHIPipeline::MaterialParameterType::Scalar: It->Kind = MaterialParameterKind::Scalar; break;
						case RHIPipeline::MaterialParameterType::Vector: It->Kind = MaterialParameterKind::Vector; break;
						case RHIPipeline::MaterialParameterType::Integer: It->Kind = MaterialParameterKind::Integer; break;
						case RHIPipeline::MaterialParameterType::Boolean: It->Kind = MaterialParameterKind::Boolean; break;
						default: break;
						}
					}
				}
				//AddResourceEvent(NewShaderPass->GetResourceEvents());
			}
		}
		mLayoutRevision.fetch_add(1, std::memory_order_acq_rel);
		return true;
	}

	void Material::UnloadImpl()
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		for (auto& [PassName, Pass] : mPassMap)
			if (Pass) ShaderPassManager::instance()->DestroyShaderPass(Pass);
		mPassMap.clear();
		mScalarDefaults.clear();
		mVectorDefaults.clear();
		mIntDefaults.clear();
		mBoolDefaults.clear();
		mParameterLayout.clear();
		for (auto& Texture : mTextures)
			Texture = TexturePtr();
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
		auto It = mPassMap.find(InPassType);
		return It == mPassMap.end() ? nullptr : It->second;
	}

	bool Material::IsRegisterPass(const Name& InPassType)
	{
		return mPassMap.find(InPassType) != mPassMap.end();
	}

	void Material::GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
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
