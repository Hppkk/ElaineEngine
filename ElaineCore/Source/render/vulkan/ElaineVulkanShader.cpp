#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanShader.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "shaderc/shaderc.hpp"
#include "ElaineFileManager.h"
#include "ElaineMD5.h"


#ifdef NDEBUG
#pragma comment(lib, "shaderc_combined.lib")
#else
#pragma comment(lib, "shaderc_shared.lib")
#endif

namespace VulkanRHI
{
	class VulkanShaderCompileManager::Private
	{
	public:
		Private()
		{
			mOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
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
			InOutResult = mCompiler.CompileGlslToSpv(InShaderString, shaderstate, InExportPath.c_str(), InEntry.c_str(), mOptions);
			if (InOutResult.GetNumErrors() > 0)
			{
				LOG_ERROR(InOutResult.GetErrorMessage());
				return false;
			}
			return true;
		}
	private:
		shaderc::Compiler mCompiler;
		shaderc::CompileOptions mOptions;
	};


	VulkanShader::VulkanShader(VulkanDevice* InDevice, const std::string& InShaderCode, EShaderStage InStage, const std::string& InHashKey)
		: mVkDevice(InDevice)
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

		ShaderCompileResult Result;
		VulkanShaderCompileManager::instance()->PreprocessShader(this);
		bool CompileSuccessed = VulkanShaderCompileManager::instance()->CompileShaderToSPV(InStage, mShaderString, mShaderStageInfo.pName, Result);

		if (CompileSuccessed)
		{
			VkShaderModuleCreateInfo CreateInfo;
			Elaine::Memory::MemoryZero(CreateInfo);
			CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			CreateInfo.codeSize = Result.mCodeSize;
			if (Result.mCodeSize != 0)
			{
				mVkSPVCode = (uint32_t*)Memory::SystemMalloc(Result.mCodeSize);
				Memory::MemoryCopy(mVkSPVCode, Result.mSPVCode, Result.mCodeSize);
				CreateInfo.pCode = mVkSPVCode;
			}
			
			vkCreateShaderModule(mVkDevice->GetDevice(), &CreateInfo, VULKAN_CPU_ALLOCATOR, &mShaderModule);
			mShaderStageInfo.module = mShaderModule;
			mbCompiled = true;
		}

	}

	VulkanShader::~VulkanShader()
	{
		if (mVkSPVCode)
		{
			Memory::SystemFree(mVkSPVCode);
			mVkSPVCode = nullptr;
		}
		Elaine::Memory::MemoryZero(mShaderStageInfo);
	}

	void VulkanShader::CopyFrom(const VulkanShader& InShader)
	{
		//mShaderCode = InShader.mShaderCode;
		//mShaderStageFlag = InShader.mShaderStageFlag;
		//mShaderStageInfo = InShader.mShaderStageInfo;
		
	}

	void VulkanShader::OnShaderCompileFinish(const ShaderCompileResult& InResult)
	{
		VkShaderModuleCreateInfo CreateInfo;
		Elaine::Memory::MemoryZero(CreateInfo);
		CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		CreateInfo.codeSize = InResult.mCodeSize;
		CreateInfo.pCode = InResult.mSPVCode;
		vkCreateShaderModule(mVkDevice->GetDevice(), &CreateInfo, VULKAN_CPU_ALLOCATOR, &mShaderModule);
		mbCompiled = true;
	}

	void VulkanShader::GetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& InOutDescriptorSetLayouts)
	{
		for (auto&& DescLayout : mDescriptorSetLayouts)
		{
			InOutDescriptorSetLayouts.push_back(DescLayout);
		}
	}

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

	VulkanShader* VulkanShaderManager::CreateShader(const std::string& InShaderCode, EShaderStage InStage)
	{
		VulkanShader* NewShader = new VulkanShader(mDevice, InShaderCode, InStage);
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

	VulkanShaderCompileManager::VulkanShaderCompileManager()
	{
		
	}

	VulkanShaderCompileManager::~VulkanShaderCompileManager()
	{
		
	}

	bool VulkanShaderCompileManager::CompileShaderToSPV(EShaderStage InShaderStage, const std::string& InShaderString, const std::string& InEntry, ShaderCompileResult& InOutResult)
	{
		VulkanShaderCompileManager::Private Comiler;
		shaderc::SpvCompilationResult Result;
		bool CompileSuccessed;
		CompileSuccessed = Comiler.CompileGLSLToSPV(InShaderStage, InShaderString, Root::instance()->getResourcePath() + Elaine::FileManager::instance()->GetShaderPath(Elaine::FileManager::FT_Shader), InEntry, Result);
		if (CompileSuccessed)
		{
			InOutResult.SetCode(Result.begin(), Result.end() - Result.begin());
		}
		return CompileSuccessed;
	}

	bool VulkanShaderCompileManager::CompileShaderToSPV(VulkanShader* InShader, ShaderCompileResult& InOutResult)
	{
		return false;
	}

	void VulkanShaderCompileManager::AddWaitCompile(VulkanShader* InShader)
	{
		if (InShader == nullptr)
			return;

		mWaitCompileShaders.insert(InShader);
	}

	void VulkanShaderCompileManager::PreprocessShader(VulkanShader* InShader)
	{

	}


	ShaderCompileResult::~ShaderCompileResult()
	{
		free(mSPVCode);
		mSPVCode = nullptr;
	}

	void ShaderCompileResult::SetCode(const uint32_t* InCode, size_t InSize)
	{
		mSPVCode = (uint32_t*)malloc(InSize * sizeof(uint32_t));
		mCodeSize = InSize * sizeof(uint32_t);
		Elaine::Memory::MemoryCopy(mSPVCode, (uint32_t*)InCode, mCodeSize);
	}

}