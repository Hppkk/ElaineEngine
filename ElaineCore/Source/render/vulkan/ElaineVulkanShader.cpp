#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanShader.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "shaderc/shaderc.hpp"
#include "spirv_cross/spirv_reflect.hpp"
#include "ElaineFileManager.h"
#include "ElaineMD5.h"
#include "ElaineMemoryMap.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"
#include "render/vulkan/ElaineVulkanDescriptorSet.h"
#include "render/vulkan/ElaineVulkanPipeline.h"


#ifdef NDEBUG
#pragma comment(lib, "shaderc_combined.lib")
#pragma comment(lib, "spirv-cross-core.lib")
#pragma comment(lib, "spirv-cross-glsl.lib")
#pragma comment(lib, "spirv-cross-reflect.lib")
#else
#pragma comment(lib, "shaderc_shared.lib")
#pragma comment(lib, "spirv-cross-cored.lib")
#pragma comment(lib, "spirv-cross-glsld.lib")
#pragma comment(lib, "spirv-cross-reflectd.lib")
#endif

namespace VulkanRHI
{
	class ShaderIncludeResolver :public shaderc::CompileOptions::IncluderInterface
	{
	public:
		shaderc_include_result* GetInclude(const char* InRequestedSource,
			shaderc_include_type InType,
			const char* InRequestingSource,
			size_t InIncludeDepth) override
		{
			std::string SPath = Root::instance()->GetResourcePath() + "shader/vulkan/" + InRequestedSource;
			MemoryMapFile mmapFile(SPath);
			mContent = (char*)Memory::SystemMalloc(mmapFile.MapSize());
			Memory::MemoryCopy(mContent, mmapFile.MapPointer(), mmapFile.MapSize());
			shaderc_include_result* Result = new shaderc_include_result;
			Result->source_name = InRequestedSource;
			Result->source_name_length = strlen(InRequestedSource);
			Result->content = (const char*)mContent;
			Result->content_length = mmapFile.MapSize();
			Result->user_data = mContent;
			return Result;
		}

		void ReleaseInclude(shaderc_include_result* InData) override
		{
			SAFE_DELETE(InData);
		}

		virtual ~ShaderIncludeResolver()
		{
			Memory::SystemFree(mContent);
		}
	private:
		char* mContent = nullptr;
	};

	class VulkanShaderCompileManager::Private
	{
	public:
		Private()
		{
			if (!mInitilizeOnce)
			{
#ifdef _DEBUG
				shaderc_optimization_level optimization_level = shaderc_optimization_level_zero;
#else
				shaderc_optimization_level optimization_level = shaderc_optimization_level_performance;
#endif
				mOptions.SetOptimizationLevel(optimization_level);
				mOptions.SetIncluder(std::make_unique<ShaderIncludeResolver>());
				mInitilizeOnce = true;
			}
		}

		bool CompileGLSLToSPV(EShaderStage InShaderStage, const std::string& InShaderString, const std::string& InExportPath, const std::string& InEntry, shaderc::SpvCompilationResult& InOutResult)
		{
			shaderc_shader_kind shaderstate = shaderc_glsl_vertex_shader;
			switch (InShaderStage)
			{
			case Elaine::EShaderStage::VertexShader:
				shaderstate = shaderc_glsl_vertex_shader;
				break;
			case Elaine::EShaderStage::GeometryShader:
				shaderstate = shaderc_geometry_shader;
				break;
			case Elaine::EShaderStage::FragmentShader:
				shaderstate = shaderc_fragment_shader;
				break;
			case Elaine::EShaderStage::ComputeShader:
				shaderstate = shaderc_compute_shader;
				break;
			}
			InOutResult = mCompiler.CompileGlslToSpv(InShaderString, shaderstate,InExportPath.c_str(), InEntry.c_str(), mOptions);
			if (InOutResult.GetNumErrors() > 0)
			{
				LOG_ERROR("{}", InOutResult.GetErrorMessage());
				return false;
			}
			return true;
		}
	private:
		shaderc::Compiler mCompiler;
		shaderc::CompileOptions mOptions;
		bool mInitilizeOnce = false;
	};


	VulkanShader::VulkanShader(VulkanDevice* InDevice, const std::string& InShaderCode, EShaderStage InStage, const std::string& InHashKey)
		: mVkDevice(InDevice)
		, mRHIShaderStage(InStage)
	{
		mShaderString = InShaderCode;
		Elaine::Memory::MemoryZero(mShaderStageInfo);

		switch (InStage)
		{
		case EShaderStage::VertexShader:
			mType = RRT_VertexShader;
			mShaderStageInfo.pName = Elaine::VS_SHADER_MAIN;
			mShaderStageFlag = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case EShaderStage::GeometryShader:
			mType = RRT_GeometryShader;
			mShaderStageInfo.pName = Elaine::GS_SHADER_MAIN;
			mShaderStageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case EShaderStage::FragmentShader:
			mType = RRT_PixelShader;
			mShaderStageInfo.pName = Elaine::PS_SHADER_MAIN;
			mShaderStageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case EShaderStage::ComputeShader:
			mType = RRT_ComputeShader;
			mShaderStageInfo.pName = Elaine::CS_SHADER_MAIN;
			mShaderStageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		default:
			break;
		}
		if (InHashKey.empty())
		{
			mHashKey = GenerateShaderHashKey(InShaderCode);
		}
		
		mShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		mShaderStageInfo.stage = mShaderStageFlag;

		//todo compile async
		//ShaderCompileResult Result;
		//VulkanShaderCompileManager::instance()->PreprocessShader(this);
		//bool CompileSuccessed = VulkanShaderCompileManager::instance()->CompileShaderToSPV(InStage, mShaderString, mShaderStageInfo.pName, Result);

		//if (CompileSuccessed)
		//{
		//	VkShaderModuleCreateInfo CreateInfo;
		//	Elaine::Memory::MemoryZero(CreateInfo);
		//	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		//	CreateInfo.codeSize = Result.mSpirvCode.size() * sizeof(uint32_t);
		//	if (CreateInfo.codeSize != 0)
		//	{
		//		mSPVCodeDesc.mSPVCodeContent = (uint32_t*)Memory::SystemMalloc(CreateInfo.codeSize);
		//		Memory::MemoryCopy(mSPVCodeDesc.mSPVCodeContent, Result.mSpirvCode.data(), CreateInfo.codeSize);
		//		CreateInfo.pCode = mSPVCodeDesc.mSPVCodeContent;
		//		mSPVCodeDesc.mContentSize = CreateInfo.codeSize;
		//	}
		//	
		//	vkCreateShaderModule(mVkDevice->GetDevice(), &CreateInfo, VULKAN_CPU_ALLOCATOR, &mShaderModule);
		//	mShaderStageInfo.module = mShaderModule;
		//	GenerateDescriptorSetLayout(Result);
		//	mInputAttributeDescriptions = Result.mReflectionData.inputAttributes;
		//	mbCompiled = true;
		//}

	}

	VulkanShader::~VulkanShader()
	{
		Elaine::Memory::MemoryZero(mShaderStageInfo);
	}

	void VulkanShader::CopyFrom(const VulkanShader& InShader)
	{
		//mShaderCode = InShader.mShaderCode;
		//mShaderStageFlag = InShader.mShaderStageFlag;
		//mShaderStageInfo = InShader.mShaderStageInfo;
		
	}

	//void VulkanShader::OnShaderCompileFinish(const ShaderCompileResult& InResult)
	//{
	//	VkShaderModuleCreateInfo CreateInfo;
	//	Elaine::Memory::MemoryZero(CreateInfo);
	//	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//	CreateInfo.codeSize = InResult.mSpirvCode.size() * sizeof(uint32_t);
	//	CreateInfo.pCode = InResult.mSpirvCode.data();
	//	vkCreateShaderModule(mVkDevice->GetDevice(), &CreateInfo, VULKAN_CPU_ALLOCATOR, &mShaderModule);
	//	mbCompiled = true;
	//}

	void VulkanShader::GetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& InOutDescriptorSetLayouts)
	{
		for (auto&& DescLayout : mDescriptorSetLayouts)
		{
			InOutDescriptorSetLayouts.push_back(DescLayout);
		}
	}

	void VulkanShader::CreateShaderModule()
	{
		VkShaderModuleCreateInfo CreateInfo;
		Elaine::Memory::MemoryZero(CreateInfo);
		CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		CreateInfo.codeSize = mSPVCodeDesc.mContentSize * sizeof(uint32_t);
		CreateInfo.pCode = mSPVCodeDesc.mSPVCodeContent;

		vkCreateShaderModule(mVkDevice->GetDevice(), &CreateInfo, VULKAN_CPU_ALLOCATOR, &mShaderModule);
		mShaderStageInfo.module = mShaderModule;
		mbCompiled = true;

	}

	//void VulkanShader::GenerateDescriptorSetLayout(const ShaderCompileResult& InResult)
	//{
	//	std::vector<uint32_t> setIndices;
	//	for (const auto& [set, _] : InResult.mReflectionData.descriptorSets)
	//	{
	//		setIndices.push_back(set);
	//	}
	//	std::sort(setIndices.begin(), setIndices.end());

	//	for (uint32_t setIndex : setIndices)
	//	{
	//		const auto& bindings = InResult.mReflectionData.descriptorSets.at(setIndex);
	//		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	//		for (const auto& binding : bindings)
	//		{
	//			VkDescriptorSetLayoutBinding layoutBinding{};
	//			layoutBinding.binding = binding.binding;
	//			layoutBinding.descriptorType = binding.type;
	//			layoutBinding.descriptorCount = binding.count;
	//			layoutBinding.stageFlags = binding.stageFlags;
	//			layoutBinding.pImmutableSamplers = nullptr;

	//			layoutBindings.push_back(layoutBinding);
	//		}

	//		VkDescriptorSetLayoutCreateInfo layoutInfo{};
	//		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	//		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	//		layoutInfo.pBindings = layoutBindings.data();
	//		//layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	//		VkDescriptorSetLayout setLayout;
	//		vkCreateDescriptorSetLayout(mVkDevice->GetDevice(), &layoutInfo, VULKAN_CPU_ALLOCATOR, &setLayout);
	//		mDescriptorSetLayouts.push_back(setLayout);
	//		VulkanCommandContext* VkCommandCtx = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());
	//		if (setIndex == 1 && !VkCommandCtx->IsCreateCommonDescriptorSets())
	//		{
	//			for (int Index = 0; Index < MAX_FRAMES_IN_FLIGHT; ++Index)
	//			{
	//				VulkanDescriptorSet* DescriptorSetRHI = new VulkanDescriptorSet();
	//				VkDescriptorSetAllocateInfo DescriptorSetAllocInfo{ };
	//				DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//				DescriptorSetAllocInfo.descriptorPool = VK_NULL_HANDLE;
	//				DescriptorSetAllocInfo.descriptorSetCount = 1;
	//				DescriptorSetAllocInfo.pSetLayouts = &setLayout;
	//				VkCommandCtx->GetDescriptorSetManager()->AllocateDescriptorSets(DescriptorSetAllocInfo, &DescriptorSetRHI);
	//				VkCommandCtx->SetCommonDescriptorSets(DescriptorSetRHI, Index);
	//			}
	//		}
	//	}
	//}

	VulkanShaderManager::VulkanShaderManager(VulkanDevice* InDevice)
		: mDevice(InDevice)
	{

	}

	VulkanShaderManager::~VulkanShaderManager()
	{

	}

	VulkanShader* VulkanShaderManager::GetOrCreateShader(const std::string& InShaderCode, EShaderStage InStage)
	{
		std::string ShaderHashKey = GenerateShaderHashKey(InShaderCode);
		auto Iter = mShaderCache.find(ShaderHashKey);
		if (Iter != mShaderCache.end())
		{
			return Iter->second;
		}
		VulkanShader* NewShader = new VulkanShader(mDevice, InShaderCode, InStage, ShaderHashKey);
		mShaderCache.emplace(ShaderHashKey, NewShader);
		return NewShader;
	}

	VulkanShader* VulkanShaderManager::CreateShader(const std::string& InShaderCode, EShaderStage InStage, VulkanPipeline* InPipeline, const std::string& InShaderPath)
	{
		VulkanShader* NewShader = new VulkanShader(mDevice, InShaderCode, InStage);
		NewShader->mPipeline = InPipeline;
		NewShader->mShaderPath = InShaderPath;
		mShaderCache.emplace(NewShader->GetHash(), NewShader);
		return NewShader;
	}

	bool VulkanShaderManager::CompileShader()
	{
		return false;
	}

	std::string GenerateShaderHashKey(const std::string& InShaderCode)
	{
		return Elaine::md5(InShaderCode);
	}

	VulkanShaderCompileManager::VulkanShaderCompileManager(VulkanDevice* InDevice)
		: mDevice(InDevice)
	{
		
	}

	VulkanShaderCompileManager::~VulkanShaderCompileManager()
	{
		
	}

	//bool VulkanShaderCompileManager::CompileShaderToSPV(EShaderStage InShaderStage, const std::string& InShaderString, const std::string& InEntry, ShaderCompileResult& InOutResult)
	//{
	//	VulkanShaderCompileManager::Private Comiler;
	//	shaderc::SpvCompilationResult Result;
	//	bool CompileSuccessed;
	//	CompileSuccessed = Comiler.CompileGLSLToSPV(InShaderStage, InShaderString, Root::instance()->getResourcePath() + Elaine::FileManager::instance()->GetShaderPath(Elaine::FileManager::FT_Shader), InEntry, Result);
	//	if (CompileSuccessed)
	//	{
	//		InOutResult.SetCode(Result.begin(), Result.end() - Result.begin());
	//		ReflectShaderModule(InOutResult, InShaderStage);
	//	}
	//	return CompileSuccessed;
	//}

	bool VulkanShaderCompileManager::CompileShaderToSPV(VulkanShader* InShader)
	{
		VulkanShaderCompileManager::Private Comiler;
		shaderc::SpvCompilationResult Result;
		bool CompileSuccessed;
		CompileSuccessed = Comiler.CompileGLSLToSPV(InShader->mRHIShaderStage, InShader->GetShaderString(), InShader->mShaderPath, InShader->mShaderStageInfo.pName, Result);
		if (CompileSuccessed)
		{
			InShader->mSPVCodeDesc.SetCode(Result.begin(), Result.end() - Result.begin());
		}
		return CompileSuccessed;
	}

	bool VulkanShaderCompileManager::PostCompileShader(VulkanShader* InVertexShader, VulkanShader* InFragmentShader)
	{
		assert(InVertexShader->mPipeline == InFragmentShader->mPipeline);

		PipelineReflectionData PipelineRefData;
		PipelineRefData.mPipeline = InVertexShader->mPipeline;

		auto ReflectShaderResourceInfo = [&](VulkanShader* InShader)
			{
				spirv_cross::CompilerReflection reflection(InShader->GetSPVCodeDesc().mSPVCodeContent, InShader->GetSPVCodeDesc().mContentSize);
				spirv_cross::ShaderResources resources = reflection.get_shader_resources();
				for (const auto& resource : resources.uniform_buffers)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					bindingInfo.name = reflection.get_name(resource.id);
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();
					const auto& type = reflection.get_type(resource.base_type_id);
					bindingInfo.blockSize = reflection.get_declared_struct_size(type);
					bindingInfo.count = 1;
					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				for (const auto& resource : resources.storage_buffers)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					bindingInfo.name = reflection.get_name(resource.id);
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();
					const auto& type = reflection.get_type(resource.base_type_id);
					bindingInfo.blockSize = reflection.get_declared_struct_size(type);
					bindingInfo.count = 1;
					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				for (const auto& resource : resources.sampled_images)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // 或 VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
					bindingInfo.name = reflection.get_name(resource.id);
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();

					const auto& type = reflection.get_type(resource.type_id);
					bindingInfo.count = type.array.empty() ? 1 : type.array[0];

					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				for (const auto& resource : resources.storage_images)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					bindingInfo.name = reflection.get_name(resource.id);
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();

					const auto& type = reflection.get_type(resource.type_id);
					bindingInfo.count = type.array.empty() ? 1 : type.array[0];

					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				for (const auto& resource : resources.separate_samplers)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_SAMPLER;
					bindingInfo.name = reflection.get_name(resource.id);
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();
					bindingInfo.count = 1;

					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				for (const auto& resource : resources.subpass_inputs)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
					bindingInfo.name = reflection.get_name(resource.id);
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();
					bindingInfo.count = 1;

					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				if (!resources.push_constant_buffers.empty())
				{
					const auto& pushConstant = resources.push_constant_buffers[0];
					const auto& type = reflection.get_type(pushConstant.base_type_id);

					PushConstantRange bindingInfo{};
					bindingInfo.offset = 0;
					bindingInfo.size = static_cast<uint32_t>(reflection.get_declared_struct_size(type));
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();
					PipelineRefData.pushConstants.push_back(bindingInfo);
				}

				if (InShader->GetVkShaderStageFlag() == VK_SHADER_STAGE_VERTEX_BIT)
				{
					//@TODO : for mutil bindings
					VkVertexInputBindingDescription TempInputBindingDesc;
					Memory::MemoryZero(TempInputBindingDesc);
					TempInputBindingDesc.binding = 0;
					TempInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

					// 1. 定义一个辅助结构来暂存信息
					struct ReflectedAttribute
					{
						uint32_t location;
						uint32_t binding;
						VkFormat format;
						uint32_t size;
						spirv_cross::ID id;
					};

					std::vector<ReflectedAttribute> sortedAttributes;

					// 2. 第一遍循环：收集信息并确定 Format
					for (const auto& resource : resources.stage_inputs)
					{
						ReflectedAttribute attribute{};
						attribute.id = resource.id;
						attribute.location = reflection.get_decoration(resource.id, spv::DecorationLocation);
						attribute.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);

						const auto& type = reflection.get_type(resource.base_type_id);
						switch (type.basetype)
						{
						case spirv_cross::SPIRType::Float:
							attribute.format = type.vecsize == 3 ? VK_FORMAT_R32G32B32_SFLOAT :
								type.vecsize == 2 ? VK_FORMAT_R32G32_SFLOAT : type.vecsize == 4 ? VK_FORMAT_R32G32B32A32_SFLOAT :
								VK_FORMAT_R32_SFLOAT;
							TempInputBindingDesc.stride += type.vecsize * 4;
							attribute.size = type.vecsize * 4;
							break;
						case spirv_cross::SPIRType::Int:
							attribute.format = type.vecsize == 3 ? VK_FORMAT_R32G32B32_SINT :
								type.vecsize == 2 ? VK_FORMAT_R32G32_SINT : type.vecsize == 4 ? VK_FORMAT_R32G32B32A32_SINT :
								VK_FORMAT_R32_SINT;
							TempInputBindingDesc.stride += type.vecsize * 4;
							attribute.size = type.vecsize * 4;
							break;
						case spirv_cross::SPIRType::Half:
							attribute.format = type.vecsize == 3 ? VK_FORMAT_R16G16B16_SFLOAT :
								type.vecsize == 2 ? VK_FORMAT_R16G16_SFLOAT : type.vecsize == 4 ? VK_FORMAT_R16G16B16A16_SFLOAT :
								VK_FORMAT_R16_SFLOAT;
							TempInputBindingDesc.stride += type.vecsize * 2;
							attribute.size = type.vecsize * 4;
							break;
						case spirv_cross::SPIRType::Short:
							attribute.format = type.vecsize == 3 ? VK_FORMAT_R16G16B16_SINT :
								type.vecsize == 2 ? VK_FORMAT_R16G16_SINT : type.vecsize == 4 ? VK_FORMAT_R16G16B16A16_SINT :
								VK_FORMAT_R16_SINT;
							TempInputBindingDesc.stride += type.vecsize * 2;
							attribute.size = type.vecsize * 4;
							break;
							// todo other types...
						default:
							break;
						}

						sortedAttributes.push_back(attribute);
					}

					std::sort(sortedAttributes.begin(), sortedAttributes.end(),
						[](const ReflectedAttribute& a, const ReflectedAttribute& b)
						{
							return a.location < b.location;
						});

					uint32_t currentOffset = 0;

					for (const auto& attr : sortedAttributes)
					{
						VkVertexInputAttributeDescription attribute{};
						attribute.location = attr.location;
						attribute.binding = attr.binding;
						attribute.format = attr.format;
						attribute.offset = currentOffset;

						PipelineRefData.inputAttributes.push_back(attribute);
						currentOffset += attr.size;
					}
					TempInputBindingDesc.stride = currentOffset;
					InVertexShader->mVertexInputBindingDescs.push_back(TempInputBindingDesc);
				}
			};

		ReflectShaderResourceInfo(InVertexShader);
		ReflectShaderResourceInfo(InFragmentShader);

		InVertexShader->mInputAttributeDescriptions = PipelineRefData.inputAttributes;

		//create shader descriptor 

		if (!GeneratePipelineDescriptors(PipelineRefData))
		{
			return false;
		}


		return true;
	}

	bool VulkanShaderCompileManager::GeneratePipelineDescriptors(PipelineReflectionData& InPipelineRefData)
	{
		std::vector<uint32_t> setIndices;
		for (const auto& [set, _] : InPipelineRefData.descriptorSets)
		{
			setIndices.push_back(set);
		}
		std::sort(setIndices.begin(), setIndices.end());
		uint32_t RealSetIndex = 0;
		for (uint32_t setIndex : setIndices)
		{
			if (setIndex != RealSetIndex)
			{
				for (;RealSetIndex < setIndex; ++RealSetIndex)
				{
					VkDescriptorSetLayoutCreateInfo EmptyLayoutInfo { };
					EmptyLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					EmptyLayoutInfo.bindingCount = 0;
					EmptyLayoutInfo.pBindings = nullptr;
					VkDescriptorSetLayout EmptySetLayout;
					vkCreateDescriptorSetLayout(mDevice->GetDevice(), &EmptyLayoutInfo, VULKAN_CPU_ALLOCATOR, &EmptySetLayout);
					InPipelineRefData.mPipeline->mDescriptorSetLayouts.push_back(EmptySetLayout);

					VulkanDescriptorSet* DescriptorSetRHI = new VulkanDescriptorSet(RealSetIndex, true);
					VkDescriptorSetAllocateInfo DescriptorSetAllocInfo{ };
					DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					DescriptorSetAllocInfo.descriptorPool = VK_NULL_HANDLE;
					DescriptorSetAllocInfo.descriptorSetCount = 1;
					DescriptorSetAllocInfo.pSetLayouts = &EmptySetLayout;
					VulkanCommandContext* VkCommandCtx = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());
					VkCommandCtx->GetDescriptorSetManager()->AllocateDescriptorSets(DescriptorSetAllocInfo, &DescriptorSetRHI);
					InPipelineRefData.mPipeline->mDescriptorSets.push_back(DescriptorSetRHI);
					InPipelineRefData.mPipeline->mDescriptorSetMap[DescriptorSetRHI->GetSet()] = DescriptorSetRHI;
				}
			}
			++RealSetIndex;

			const auto& bindings = InPipelineRefData.descriptorSets.at(setIndex);
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			for (const auto& binding : bindings)
			{
				VkDescriptorSetLayoutBinding layoutBinding{};
				layoutBinding.binding = binding.binding;
				layoutBinding.descriptorType = binding.type;
				layoutBinding.descriptorCount = binding.count;
				layoutBinding.stageFlags = binding.stageFlags;
				layoutBinding.pImmutableSamplers = nullptr;

				layoutBindings.push_back(layoutBinding);
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
			layoutInfo.pBindings = layoutBindings.data();
			//layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VkDescriptorSetLayout setLayout;
			vkCreateDescriptorSetLayout(mDevice->GetDevice(), &layoutInfo, VULKAN_CPU_ALLOCATOR, &setLayout);
			InPipelineRefData.mPipeline->mDescriptorSetLayouts.push_back(setLayout);
			VulkanCommandContext* VkCommandCtx = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());
			if (setIndex == 0 && !VkCommandCtx->IsCreateCommonDescriptorSets())
			{
				for (int Index = 0; Index < MAX_FRAMES_IN_FLIGHT; ++Index)
				{
					VulkanDescriptorSet* DescriptorSetRHI = new VulkanDescriptorSet(setIndex);
					VkDescriptorSetAllocateInfo DescriptorSetAllocInfo{ };
					DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					DescriptorSetAllocInfo.descriptorPool = VK_NULL_HANDLE;
					DescriptorSetAllocInfo.descriptorSetCount = 1;
					DescriptorSetAllocInfo.pSetLayouts = &setLayout;
					VkCommandCtx->GetDescriptorSetManager()->AllocateDescriptorSets(DescriptorSetAllocInfo, &DescriptorSetRHI);
					VkCommandCtx->SetCommonDescriptorSets(DescriptorSetRHI, Index);
					InPipelineRefData.mPipeline->mDescriptorSetMap[DescriptorSetRHI->GetSet()] = DescriptorSetRHI;
				}
			}
			else if (setIndex != 0)
			{
				VulkanDescriptorSet* DescriptorSetRHI = new VulkanDescriptorSet(setIndex);
				VkDescriptorSetAllocateInfo DescriptorSetAllocInfo{ };
				DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				DescriptorSetAllocInfo.descriptorPool = VK_NULL_HANDLE;
				DescriptorSetAllocInfo.descriptorSetCount = 1;
				DescriptorSetAllocInfo.pSetLayouts = &setLayout;
				VkCommandCtx->GetDescriptorSetManager()->AllocateDescriptorSets(DescriptorSetAllocInfo, &DescriptorSetRHI);
				InPipelineRefData.mPipeline->mDescriptorSets.push_back(DescriptorSetRHI);
				InPipelineRefData.mPipeline->mDescriptorSetMap[DescriptorSetRHI->GetSet()] = DescriptorSetRHI;
			}
		}
		return true;
	}

	void VulkanShaderCompileManager::AddWaitCompile(VulkanShader* InShader)
	{
		if (InShader == nullptr)
			return;

		mWaitCompileShaders.insert(InShader);
	}

	bool VulkanShaderCompileManager::CompilePipeline(VulkanPipeline* InPipeline)
	{
		if (InPipeline == nullptr)
			return false;

		if (InPipeline->GetType() == VulkanPipeline::Graphic)
		{
			VulkanGfxPipeline* CurrentPipeline = static_cast<VulkanGfxPipeline*>(InPipeline);
			PreprocessShader(CurrentPipeline->mVsShader);
			if (!CompileShaderToSPV(CurrentPipeline->mVsShader))
			{
				return false;
			}
			CurrentPipeline->mVsShader->CreateShaderModule();

			PreprocessShader(CurrentPipeline->mPsShader);
			if (!CompileShaderToSPV(CurrentPipeline->mPsShader))
			{
				return false;
			}
			CurrentPipeline->mPsShader->CreateShaderModule();

			return PostCompileShader(CurrentPipeline->mVsShader, CurrentPipeline->mPsShader);

		}
		else
		{
			VulkanComputePipeline* CurrentPipeline = static_cast<VulkanComputePipeline*>(InPipeline);


		}

		return true;
	}

//#define GEN_SHADER_STAGE \
//			switch (InShaderStage)\
//			{\
//			case Elaine::EShaderStage::VertexShader:\
//				bindingInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;\
//				break;\
//			case Elaine::EShaderStage::GeometryShader:\
//				bindingInfo.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;\
//				break;\
//			case Elaine::EShaderStage::FragmentShader:\
//				bindingInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;\
//				break;\
//			case Elaine::EShaderStage::ComputeShader:\
//				bindingInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;\
//				break;\
//			}
//
//
//	void VulkanShaderCompileManager::ReflectShaderModule(ShaderCompileResult& InOutResult, EShaderStage InShaderStage)
//	{
//		spirv_cross::CompilerReflection reflection(InOutResult.mSpirvCode);
//		spirv_cross::ShaderResources resources = reflection.get_shader_resources();
//		for (const auto& resource : resources.uniform_buffers)
//		{
//			DescriptorBindingInfo bindingInfo{};
//			bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
//			bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
//			bindingInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//			bindingInfo.name = resource.name;//reflection.get_name(resource.id);
//			GEN_SHADER_STAGE;
//			const auto& type = reflection.get_type(resource.base_type_id);
//			bindingInfo.blockSize = reflection.get_declared_struct_size(type);
//			bindingInfo.count = 1;
//			InOutResult.mReflectionData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
//		}
//
//		for (const auto& resource : resources.storage_buffers)
//		{
//			DescriptorBindingInfo bindingInfo{};
//			bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
//			bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
//			bindingInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//			bindingInfo.name = reflection.get_name(resource.id);
//			GEN_SHADER_STAGE;
//
//			const auto& type = reflection.get_type(resource.base_type_id);
//			bindingInfo.blockSize = reflection.get_declared_struct_size(type);
//			bindingInfo.count = 1;
//
//			InOutResult.mReflectionData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
//		}
//
//		if (!resources.push_constant_buffers.empty())
//		{
//			const auto& pushConstant = resources.push_constant_buffers[0];
//			const auto& type = reflection.get_type(pushConstant.base_type_id);
//
//			PushConstantRange bindingInfo{};
//			bindingInfo.offset = 0;
//			bindingInfo.size = static_cast<uint32_t>(reflection.get_declared_struct_size(type));
//			GEN_SHADER_STAGE;
//
//			InOutResult.mReflectionData.pushConstants.push_back(bindingInfo);
//		}
//
//		if (InShaderStage == EShaderStage::VertexShader)
//		{
//			uint32_t attributeIndex = 0;
//			for (const auto& resource : resources.stage_inputs)
//			{
//				VkVertexInputAttributeDescription attribute{};
//				attribute.location = reflection.get_decoration(resource.id, spv::DecorationLocation);
//				attribute.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
//
//				const auto& type = reflection.get_type(resource.base_type_id);
//				switch (type.basetype)
//				{
//				case spirv_cross::SPIRType::Float:
//					attribute.format = type.vecsize == 3 ? VK_FORMAT_R32G32B32_SFLOAT :
//						type.vecsize == 2 ? VK_FORMAT_R32G32_SFLOAT : type.vecsize == 4 ? VK_FORMAT_R32G32B32A32_SFLOAT :
//						VK_FORMAT_R32_SFLOAT;
//					break;
//				case spirv_cross::SPIRType::Int:
//					attribute.format = type.vecsize == 3 ? VK_FORMAT_R32G32B32_SINT :
//						type.vecsize == 2 ? VK_FORMAT_R32G32_SINT : type.vecsize == 4 ? VK_FORMAT_R32G32B32A32_SINT :
//						VK_FORMAT_R32_SINT;
//					break;
//				case spirv_cross::SPIRType::Half:
//					attribute.format = type.vecsize == 3 ? VK_FORMAT_R16G16B16_SFLOAT :
//						type.vecsize == 2 ? VK_FORMAT_R16G16_SFLOAT : type.vecsize == 4 ? VK_FORMAT_R16G16B16A16_SFLOAT :
//						VK_FORMAT_R16_SFLOAT;
//					break;
//				case spirv_cross::SPIRType::Short:
//					attribute.format = type.vecsize == 3 ? VK_FORMAT_R16G16B16_SINT :
//						type.vecsize == 2 ? VK_FORMAT_R16G16_SINT : type.vecsize == 4 ? VK_FORMAT_R16G16B16A16_SINT :
//						VK_FORMAT_R16_SINT;
//					break;
//				// todo other types...
//				default:
//					break;
//				}
//
//				attribute.offset = reflection.get_decoration(resource.id, spv::DecorationOffset);
//				InOutResult.mReflectionData.inputAttributes.push_back(attribute);
//			}
//		}
//
//	}
//#undef GEN_SHADER_STAGE

	void VulkanShaderCompileManager::PreprocessShader(VulkanShader* InShader)
	{
		InShader->mShaderString.insert(0, "#extension GL_ARB_separate_shader_objects : enable\n");
		InShader->mShaderString.insert(0, "#version 450 core\n");
	}


	//ShaderCompileResult::~ShaderCompileResult()
	//{

	//}

	//void ShaderCompileResult::SetCode(const uint32_t* InCode, size_t InSize)
	//{
	//	mSpirvCode.resize(InSize);
	//	Elaine::Memory::MemoryCopy(mSpirvCode.data(), (uint32_t*)InCode, InSize * sizeof(uint32_t));
	//}

	SPVCodeDesc::~SPVCodeDesc()
	{
		if (mSPVCodeContent)
		{
			Memory::SystemFree(mSPVCodeContent);
			mSPVCodeContent = nullptr;
			mContentSize = 0u;
		}
	}

	void SPVCodeDesc::SetCode(const uint32_t* InCode, size_t InSize)
	{
		mSPVCodeContent = (uint32_t*)Memory::SystemMalloc(InSize * sizeof(uint32_t));
		Memory::MemoryCopy(mSPVCodeContent, InCode, InSize * sizeof(uint32_t));
		mContentSize = InSize;
	}

}