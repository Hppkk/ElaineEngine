#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanShader.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "shaderc/shaderc.hpp"
#include "render/ElaineShaderParameters.h"
#include "spirv_cross/spirv_reflect.hpp"
#include "ElaineFileManager.h"
#include "ElaineMD5.h"
#include "ElaineHash.h"
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
        struct Include { std::string Name, Content; };
	public:
        explicit ShaderIncludeResolver(std::vector<std::string>& Files, std::vector<std::string>& Generated, uint64_t& Signature)
            : mFiles(Files), mGenerated(Generated), mSignature(Signature) {}
		shaderc_include_result* GetInclude(const char* InRequestedSource,
			shaderc_include_type InType,
			const char* InRequestingSource,
			size_t InIncludeDepth) override
		{
            auto* Data = new Include;
            const std::string Requested(InRequestedSource);
            if (Requested.find("Generated/") == 0)
            {
                Data->Name = Requested;
                uint64_t Signature = 0;
                if (!GetGeneratedShaderInclude(Requested, Data->Content, Signature)) Data->Content = "#error Unknown generated shader include";
                else if (std::find(mGenerated.begin(), mGenerated.end(), Requested) == mGenerated.end())
                { mGenerated.push_back(Requested); mSignature ^= Signature + 0x9e3779b97f4a7c15ull + (mSignature << 6) + (mSignature >> 2); }
                auto* Result = new shaderc_include_result;
                Result->source_name = Data->Name.c_str(); Result->source_name_length = Data->Name.size();
                Result->content = Data->Content.c_str(); Result->content_length = Data->Content.size(); Result->user_data = Data;
                return Result;
            }
            const auto RootPath = std::filesystem::path(Root::instance()->GetResourcePath()) / "shader/vulkan";
            auto Path = RootPath / InRequestedSource;
            if (InType == shaderc_include_type_relative)
            {
                auto Parent = std::filesystem::path(InRequestingSource).parent_path();
                if (!Parent.is_absolute()) Parent = RootPath / Parent;
                const auto Relative = Parent / InRequestedSource;
                if (std::filesystem::exists(Relative)) Path = Relative;
            }
            Data->Name = Path.lexically_normal().generic_string();
            mFiles.push_back(Data->Name);
            std::error_code FileError;
            const auto FileSize = std::filesystem::file_size(Path, FileError);
            if (FileError) Data->Content = "#error Shader include was not found";
            else if (FileSize != 0)
            {
                MemoryMapFile File(Data->Name);
                if (File.MapPointer()) Data->Content.assign(static_cast<const char*>(File.MapPointer()), File.MapSize());
                else Data->Content = "#error Shader include could not be mapped";
            }
            auto* Result = new shaderc_include_result;
            Result->source_name = Data->Name.c_str(); Result->source_name_length = Data->Name.size();
            Result->content = Data->Content.c_str(); Result->content_length = Data->Content.size();
            Result->user_data = Data;
			return Result;
		}

		void ReleaseInclude(shaderc_include_result* InData) override
		{
            delete static_cast<Include*>(InData->user_data);
			SAFE_DELETE(InData);
		}

		virtual ~ShaderIncludeResolver()
		{
			Memory::SystemFree(mContent);
		}
	private:
		char* mContent = nullptr;
        std::vector<std::string>& mFiles;
        std::vector<std::string>& mGenerated;
        uint64_t& mSignature;
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
                // Names are part of the binding contract, including optimized builds.
                mOptions.SetGenerateDebugInfo();
				mOptions.SetIncluder(std::make_unique<ShaderIncludeResolver>(SourceFiles, GeneratedIncludes, DeclarationSignature));
				mInitilizeOnce = true;
			}
		}

        std::vector<std::string> SourceFiles, GeneratedIncludes;
        uint64_t DeclarationSignature = 0;
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
        if (mShaderModule && mVkDevice) vkDestroyShaderModule(mVkDevice->GetDevice(), mShaderModule, VULKAN_CPU_ALLOCATOR);
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

        if (vkCreateShaderModule(mVkDevice->GetDevice(), &CreateInfo, VULKAN_CPU_ALLOCATOR, &mShaderModule) != VK_SUCCESS) throw std::runtime_error("Shader module creation failed");
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

	VulkanShader* VulkanShaderManager::CreateShader(const std::string& InShaderCode, EShaderStage InStage, VulkanPipeline* InPipeline, const std::string& InShaderPath, std::shared_ptr<const PreparedShader> Prepared)
	{
		VulkanShader* NewShader = new VulkanShader(mDevice, InShaderCode, InStage);
        NewShader->mPrepared = std::move(Prepared);
		NewShader->mPipeline = InPipeline;
		NewShader->mShaderPath = InShaderPath;

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

    bool VulkanShaderCompileManager::CompileShaderToSPV(VulkanShader* Shader)
    {
        if (!Shader->mPrepared) return false;
        Shader->mSPVCodeDesc.SetCode(Shader->mPrepared->Spirv.data(), Shader->mPrepared->Spirv.size());
        return true;
    }

    void VulkanShaderCompileManager::ReflectCPU(VulkanShader* InShader, PipelineReflectionData& PipelineRefData)
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
                    if (bindingInfo.name.empty()) bindingInfo.name = resource.name;
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();
					const auto& type = reflection.get_type(resource.base_type_id);
					bindingInfo.blockSize = reflection.get_declared_struct_size(type);
					bindingInfo.count = 1;
                    if (!reflection.get_type(resource.type_id).array.empty()) throw std::runtime_error("Uniform descriptor arrays are not supported: " + bindingInfo.name);
                    auto& Layout = bindingInfo.ResourceLayout;
                    Layout.Name = bindingInfo.name; Layout.Group = bindingInfo.set; Layout.Binding = bindingInfo.binding;
                    Layout.Size = static_cast<uint32>(bindingInfo.blockSize); Layout.Type = RHIShaderResourceType::Uniform;
                    for (uint32 Member = 0; Member < type.member_types.size(); ++Member)
                    {
                        const auto& T = reflection.get_type(type.member_types[Member]);
                        if (T.width != 32 && T.basetype != spirv_cross::SPIRType::Boolean)
                            throw std::runtime_error("Only 32-bit uniform components are supported: " + reflection.get_member_name(resource.base_type_id, Member));
                        RHIShaderMember M; M.Name = reflection.get_member_name(resource.base_type_id, Member);
                        M.Offset = reflection.type_struct_member_offset(type, Member);
                        M.Size = static_cast<uint32>(reflection.get_declared_struct_member_size(type, Member));
                        M.Rows = T.vecsize; M.Columns = T.columns;
                        if (T.basetype == spirv_cross::SPIRType::Float && T.columns > 1) M.Type = RHIShaderValueType::Matrix;
                        else if (T.basetype == spirv_cross::SPIRType::Float) M.Type = RHIShaderValueType::Float;
                        else if (T.basetype == spirv_cross::SPIRType::Boolean) M.Type = RHIShaderValueType::Boolean;
                        else if (T.basetype == spirv_cross::SPIRType::Int) M.Type = RHIShaderValueType::Integer;
                        else if (T.basetype == spirv_cross::SPIRType::UInt) M.Type = RHIShaderValueType::UnsignedInteger;
                        else throw std::runtime_error("Unsupported uniform member type: " + M.Name);
                        if (!T.array.empty())
                        {
                            if (T.array.size() != 1 || !T.array_size_literal[0] || !T.array[0]) throw std::runtime_error("Unsupported uniform array: " + M.Name);
                            M.ArrayCount = T.array[0]; M.ArrayStride = reflection.type_struct_member_array_stride(type, Member);
                        }
                        if (T.columns > 1)
                        {
                            M.MatrixStride = reflection.type_struct_member_matrix_stride(type, Member);
                            M.RowMajor = reflection.has_member_decoration(resource.base_type_id, Member, spv::DecorationRowMajor);
                        }
                        Layout.Members.push_back(std::move(M));
                    }
					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
					if (bindingInfo.set == static_cast<uint32>(RHIResourceGroup::PerMaterial))
					{
						for (uint32_t MemberIndex = 0; MemberIndex < type.member_types.size(); ++MemberIndex)
						{
							RHIPipeline::MaterialParameterInfo Parameter;
							Parameter.Name = reflection.get_member_name(resource.base_type_id, MemberIndex);
							Parameter.NameHash = Elaine::FNV_1A_HASH_64(Parameter.Name);
							Parameter.Offset = reflection.type_struct_member_offset(type, MemberIndex);
							Parameter.Size = static_cast<uint32_t>(reflection.get_declared_struct_member_size(type, MemberIndex));
							Parameter.Set = bindingInfo.set;
							Parameter.Binding = bindingInfo.binding;
							const auto& MemberType = reflection.get_type(type.member_types[MemberIndex]);
							Parameter.ArrayCount = MemberType.array.empty() ? 1u : static_cast<uint32_t>(MemberType.array[0]);
							if (!MemberType.array.empty())
								Parameter.Type = RHIPipeline::MaterialParameterType::Array;
							else if (MemberType.basetype == spirv_cross::SPIRType::Boolean)
								Parameter.Type = RHIPipeline::MaterialParameterType::Boolean;
							else if (MemberType.basetype == spirv_cross::SPIRType::Int || MemberType.basetype == spirv_cross::SPIRType::UInt)
								Parameter.Type = RHIPipeline::MaterialParameterType::Integer;
							else if (MemberType.basetype == spirv_cross::SPIRType::Float && MemberType.vecsize == 1 && MemberType.columns == 1)
								Parameter.Type = RHIPipeline::MaterialParameterType::Scalar;
							else if (MemberType.basetype == spirv_cross::SPIRType::Float && MemberType.columns > 1)
								Parameter.Type = RHIPipeline::MaterialParameterType::Matrix;
							else if (MemberType.basetype == spirv_cross::SPIRType::Float)
								Parameter.Type = RHIPipeline::MaterialParameterType::Vector;
							auto Existing = std::find_if(PipelineRefData.materialParameters.begin(), PipelineRefData.materialParameters.end(),
								[&Parameter](const RHIPipeline::MaterialParameterInfo& Item) { return Item.NameHash == Parameter.NameHash && Item.Binding == Parameter.Binding; });
							if (Existing == PipelineRefData.materialParameters.end())
								PipelineRefData.materialParameters.push_back(std::move(Parameter));
						}
					}
				}

				for (const auto& resource : resources.storage_buffers)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					bindingInfo.name = reflection.get_name(resource.id);
                    if (bindingInfo.name.empty()) bindingInfo.name = resource.name;
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();
					const auto& type = reflection.get_type(resource.base_type_id);
					bindingInfo.blockSize = reflection.get_declared_struct_size(type);
					bindingInfo.count = 1;
                    if (!reflection.get_buffer_block_flags(resource.id).get(spv::DecorationNonWritable))
                        throw std::runtime_error("Writable storage buffers are not supported: " + bindingInfo.name);
                    if (!reflection.get_type(resource.type_id).array.empty()) throw std::runtime_error("Buffer descriptor arrays are not supported: " + bindingInfo.name);
                    auto& Layout = bindingInfo.ResourceLayout;
                    Layout.Name = bindingInfo.name; Layout.Group = bindingInfo.set; Layout.Binding = bindingInfo.binding;
                    Layout.Size = static_cast<uint32>(bindingInfo.blockSize); Layout.Type = RHIShaderResourceType::ReadOnlyBuffer;
                    if (!type.member_types.empty() && !reflection.get_type(type.member_types.back()).array.empty())
                        Layout.ElementStride = reflection.type_struct_member_array_stride(type, static_cast<uint32>(type.member_types.size() - 1));
                    // Fixed parameter buffers declare a single array of plain value structs.
                    // Preserve its element layout so hot reload validates more than the descriptor kind.
                    if (type.member_types.size() == 1 && reflection.type_struct_member_offset(type, 0) == 0)
                    {
                        const auto& Element = reflection.get_type(type.member_types.front());
                        if (Element.basetype == spirv_cross::SPIRType::Struct && Element.array.size() == 1)
                            for (uint32 I = 0; I < Element.member_types.size(); ++I)
                            {
                                const auto& T = reflection.get_type(Element.member_types[I]);
                                if (T.width != 32 || T.basetype == spirv_cross::SPIRType::Struct) continue;
                                RHIShaderMember M;
                                M.Name = reflection.get_member_name(Element.self, I);
                                M.Offset = reflection.type_struct_member_offset(Element, I);
                                M.Size = static_cast<uint32>(reflection.get_declared_struct_member_size(Element, I));
                                M.Rows = T.vecsize; M.Columns = T.columns;
                                if (T.basetype == spirv_cross::SPIRType::Float) M.Type = T.columns > 1 ? RHIShaderValueType::Matrix : RHIShaderValueType::Float;
                                else if (T.basetype == spirv_cross::SPIRType::Int) M.Type = RHIShaderValueType::Integer;
                                else if (T.basetype == spirv_cross::SPIRType::UInt) M.Type = RHIShaderValueType::UnsignedInteger;
                                else if (T.basetype == spirv_cross::SPIRType::Boolean) M.Type = RHIShaderValueType::Boolean;
                                else continue;
                                if (!T.array.empty())
                                {
                                    if (T.array.size() != 1 || !T.array_size_literal[0] || !T.array[0]) continue;
                                    M.ArrayCount = T.array[0]; M.ArrayStride = reflection.type_struct_member_array_stride(Element, I);
                                }
                                if (T.columns > 1)
                                {
                                    M.MatrixStride = reflection.type_struct_member_matrix_stride(Element, I);
                                    M.RowMajor = reflection.has_member_decoration(Element.self, I, spv::DecorationRowMajor);
                                }
                                Layout.Members.push_back(std::move(M));
                            }
                    }
					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				for (const auto& resource : resources.sampled_images)
				{
					DescriptorBindingInfo bindingInfo{};
                    const auto& Image = reflection.get_type(resource.base_type_id).image;
                    bindingInfo.isColor2DSampler = Image.dim == spv::Dim2D && !Image.arrayed && !Image.ms &&
                        reflection.get_type(Image.type).basetype == spirv_cross::SPIRType::Float && !Image.depth;
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // �� VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
					bindingInfo.name = reflection.get_name(resource.id);
                    if (bindingInfo.name.empty()) bindingInfo.name = resource.name;
					bindingInfo.stageFlags = InShader->GetVkShaderStageFlag();

					const auto& type = reflection.get_type(resource.type_id);
					bindingInfo.count = type.array.empty() ? 1 : type.array[0];

                    auto& Layout = bindingInfo.ResourceLayout;
                    Layout.Name = bindingInfo.name; Layout.Group = bindingInfo.set; Layout.Binding = bindingInfo.binding;
                    Layout.Type = RHIShaderResourceType::Texture;
                    Layout.IntegerSample = reflection.get_type(Image.type).basetype != spirv_cross::SPIRType::Float;
                    Layout.SignedSample = reflection.get_type(Image.type).basetype == spirv_cross::SPIRType::Int;
                    Layout.DepthSample = Image.depth; Layout.Multisampled = Image.ms;
                    if (Image.dim == spv::Dim2D && !Image.arrayed) Layout.Dimension = TextureDimension::Texture2D;
                    else if (Image.dim == spv::Dim2D && Image.arrayed) Layout.Dimension = TextureDimension::Texture2DArray;
                    else if (Image.dim == spv::DimCube && !Image.arrayed) Layout.Dimension = TextureDimension::TextureCube;
                    else throw std::runtime_error("Unsupported sampled image dimension: " + bindingInfo.name);
					PipelineRefData.descriptorSets[bindingInfo.set].push_back(bindingInfo);
				}

				for (const auto& resource : resources.storage_images)
				{
					DescriptorBindingInfo bindingInfo{};
					bindingInfo.set = reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
					bindingInfo.binding = reflection.get_decoration(resource.id, spv::DecorationBinding);
					bindingInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					bindingInfo.name = reflection.get_name(resource.id);
                    if (bindingInfo.name.empty()) bindingInfo.name = resource.name;
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
                    if (bindingInfo.name.empty()) bindingInfo.name = resource.name;
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
                    if (bindingInfo.name.empty()) bindingInfo.name = resource.name;
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

					// 1. ����һ�������ṹ���ݴ���Ϣ
					struct ReflectedAttribute
					{
						uint32_t location;
						uint32_t binding;
						VkFormat format;
						uint32_t size;
						spirv_cross::ID id;
					};

					std::vector<ReflectedAttribute> sortedAttributes;

					// 2. ��һ��ѭ�����ռ���Ϣ��ȷ�� Format
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
					if (!sortedAttributes.empty()) InShader->mVertexInputBindingDescs.push_back(TempInputBindingDesc);
				}

    }
    std::shared_ptr<const PreparedShader> VulkanShaderCompileManager::PrepareCPU(const std::string& Code, const std::string& Path, EShaderStage Stage)
    {
        Private Compiler;
        shaderc::SpvCompilationResult Result;
        const char* Entry = Stage == EShaderStage::VertexShader ? VS_SHADER_MAIN : Stage == EShaderStage::FragmentShader ? PS_SHADER_MAIN : Stage == EShaderStage::ComputeShader ? CS_SHADER_MAIN : GS_SHADER_MAIN;
        const auto Source = Code.find("#version") == std::string::npos ? "#version 450 core\n#extension GL_ARB_separate_shader_objects : enable\n" + Code : Code;
        if (!Compiler.CompileGLSLToSPV(Stage, Source, Path, Entry, Result)) throw std::runtime_error(Result.GetErrorMessage());
        auto Prepared = std::make_shared<PreparedShader>();
        Prepared->Spirv.assign(Result.begin(), Result.end());
        Prepared->SourceFiles = std::move(Compiler.SourceFiles);
        Prepared->GeneratedIncludes = std::move(Compiler.GeneratedIncludes);
        Prepared->DeclarationSignature = Compiler.DeclarationSignature;
        VulkanShader Shader(nullptr, Code, Stage);
        Shader.mSPVCodeDesc.SetCode(Prepared->Spirv.data(), Prepared->Spirv.size());
        ReflectCPU(&Shader, Prepared->Reflection);
        for (const auto& Set : Prepared->Reflection.descriptorSets) for (const auto& B : Set.second) Prepared->ResourceLayout.push_back(B.ResourceLayout);
        ValidateShaderParameterLayouts(Prepared->ResourceLayout, Prepared->GeneratedIncludes);
        Prepared->InputBindings = Shader.mVertexInputBindingDescs;
        return Prepared;
    }
    bool VulkanShaderCompileManager::PostCompileShader(VulkanShader* VS, VulkanShader* PS)
    {
        if (!VS->mPrepared || !PS->mPrepared) return false;
        PipelineReflectionData Data = VS->mPrepared->Reflection;
        Data.mPipeline = VS->mPipeline;
        for (const auto& Set : PS->mPrepared->Reflection.descriptorSets)
            for (const auto& Binding : Set.second)
            {
                auto& Bindings = Data.descriptorSets[Set.first];
                auto It = std::find_if(Bindings.begin(), Bindings.end(), [&](const DescriptorBindingInfo& B) { return B.binding == Binding.binding; });
                if (It == Bindings.end()) Bindings.push_back(Binding);
                else
                {
                    if (It->type != Binding.type || It->count != Binding.count || !(It->ResourceLayout == Binding.ResourceLayout))
                        throw std::runtime_error("Shader descriptor declarations disagree between stages: " + Binding.name);
                    It->stageFlags |= Binding.stageFlags;
                }
            }
        for (auto& Parameter : PS->mPrepared->Reflection.materialParameters)
        {
            auto It = std::find_if(Data.materialParameters.begin(), Data.materialParameters.end(), [&](const RHIPipeline::MaterialParameterInfo& P) { return P.Set == Parameter.Set && P.Binding == Parameter.Binding && P.Offset == Parameter.Offset; });
            if (It == Data.materialParameters.end()) Data.materialParameters.push_back(Parameter);
        }
        std::vector<RHIShaderResourceLayout> MergedLayout;
        for (const auto& Set : Data.descriptorSets) for (const auto& Binding : Set.second) MergedLayout.push_back(Binding.ResourceLayout);
        ValidateShaderParameterLayouts(MergedLayout, VS->mPrepared->GeneratedIncludes);
        ValidateShaderParameterLayouts(MergedLayout, PS->mPrepared->GeneratedIncludes);
        VS->mInputAttributeDescriptions = Data.inputAttributes;
        VS->mVertexInputBindingDescs = VS->mPrepared->InputBindings;
        if (!GeneratePipelineDescriptors(Data)) return false;
        Data.mPipeline->mMaterialParameters = Data.materialParameters;
        Data.mPipeline->mMaterialParametersRHI = Data.materialParameters;
        return true;
    }

    bool VulkanShaderCompileManager::GeneratePipelineDescriptors(PipelineReflectionData& Data)
    {
        std::vector<RHIShaderResourceLayout> ResourceLayout;
        std::unordered_map<std::string, bool> Names;
        for (const auto& Set : Data.descriptorSets) for (const auto& Binding : Set.second)
        {
            if (Binding.name.empty()) throw std::runtime_error("Shader resource names must be retained for automatic binding");
            if (Set.first >= static_cast<uint32>(RHIResourceGroup::Count) || Binding.count != 1)
                throw std::runtime_error("Unsupported descriptor group or array: " + Binding.name);
            if (Binding.type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && Binding.type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && Binding.type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                throw std::runtime_error("Unsupported graphics descriptor type: " + Binding.name);
            if (!Names.emplace(std::to_string(Set.first) + ":" + Binding.name, true).second)
                throw std::runtime_error("Duplicate shader resource name: " + Binding.name);
            ResourceLayout.push_back(Binding.ResourceLayout);
        }
        std::sort(ResourceLayout.begin(), ResourceLayout.end(), [](const RHIShaderResourceLayout& A, const RHIShaderResourceLayout& B) {
            return A.Group != B.Group ? A.Group < B.Group : A.Binding < B.Binding;
        });
        Data.mPipeline->SetResourceLayout(std::move(ResourceLayout));
        Data.mPipeline->mDescriptorBindings = Data.descriptorSets;
        uint32 MaxSet = 0;
        for (const auto& Set : Data.descriptorSets) MaxSet = std::max(MaxSet, Set.first);
        if (MaxSet >= 32) throw std::runtime_error("Shader descriptor set exceeds supported range");
        for (uint32 Set = 0; Set <= MaxSet; ++Set)
        {
            std::vector<VkDescriptorSetLayoutBinding> Bindings;
            for (const auto& Binding : Data.descriptorSets[Set])
                Bindings.push_back({Binding.binding, Binding.type, Binding.count, Binding.stageFlags, nullptr});
            VkDescriptorSetLayoutCreateInfo Info{}; Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            Info.bindingCount = static_cast<uint32>(Bindings.size()); Info.pBindings = Bindings.data();
            VkDescriptorSetLayout Layout;
            if (vkCreateDescriptorSetLayout(mDevice->GetDevice(), &Info, VULKAN_CPU_ALLOCATOR, &Layout) != VK_SUCCESS) return false;
            Data.mPipeline->mDescriptorSetLayouts.push_back(Layout);
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

			if (!CompileShaderToSPV(CurrentPipeline->mVsShader))
			{
				return false;
			}
			CurrentPipeline->mVsShader->CreateShaderModule();


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
