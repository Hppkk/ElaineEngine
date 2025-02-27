#pragma once
#include "render/vulkan/ElaineVulkanTypes.h"

namespace VulkanRHI
{
	class VulkanDevice;

	struct ElaineCoreExport ShaderCompileResult
	{
		ShaderCompileResult() = default;
		~ShaderCompileResult();

		void SetCode(const uint32_t* InCode, size_t InSize);

		uint32_t* mSPVCode = nullptr;
		size_t mCodeSize = 0;
	};

	class ShaderCompileFinishListener
	{
	public:
		virtual void OnShaderCompileFinish(const ShaderCompileResult& InResult) = 0;
	};

	std::string GenerateShaderHashKey(const std::string& InShaderCode);

	class ElaineCoreExport VulkanShader :public Elaine::RHIShader, ShaderCompileFinishListener
	{
	public:
		VulkanShader(VulkanDevice* InDevice, const std::string& InShaderCode, EShaderStage InStage, const std::string& InHashKey = "");
		~VulkanShader();
		std::string GetHash() const { return mHashKey; }
		const VkPipelineShaderStageCreateInfo& GetShaderStageCreateInfo() const { return mShaderStageInfo; }
		void CopyFrom(const VulkanShader& InShader);
		virtual void OnShaderCompileFinish(const ShaderCompileResult& InResult) override;
		bool IsShaderCompiled() const { return mbCompiled; }
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return mDescriptorSetLayouts; }
		const std::vector<VkDescriptorSet>& GetDescriptorSet() const { return mDescriptorSets; }
		void GetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& InOutDescriptorSetLayouts);
	protected:
		std::string mHashKey;
		VkShaderModule mShaderModule = nullptr;
		VkShaderStageFlagBits mShaderStageFlag = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		VkPipelineShaderStageCreateInfo mShaderStageInfo;
		VulkanDevice* mVkDevice = nullptr;
		uint32_t* mVkSPVCode = nullptr;
		std::vector<VkDescriptorSet> mDescriptorSets;
		std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
		bool mbCompiled = false;
	};


	class ElaineCoreExport VulkanShaderManager
	{
	public:
		VulkanShaderManager(VulkanDevice* InDevice);
		~VulkanShaderManager();
		VulkanShader* GetOrCreateShader(const std::string& InShaderCode, EShaderStage InStage);
		VulkanShader* CreateShader(const std::string& InShaderCode, EShaderStage InStage);
		bool CompileShader();
	private:
		std::map<std::string, VulkanShader*> mShaderCache;
		VulkanDevice* mDevice = nullptr;
	};

	//class ElaineCoreExport VulkanGraphicShader : public VulkanShader
	//{
	//public:
	//};

	//class ElaineCoreExport VulkanComputeShader : public VulkanShader
	//{
	//public:
	//};




	class ElaineCoreExport VulkanShaderCompileManager :public Elaine::Singleton<VulkanShaderCompileManager>
	{
	public:
		class Private;
	public:
		VulkanShaderCompileManager();
		~VulkanShaderCompileManager();
		void PreprocessShader(VulkanShader* InShader);
		bool CompileShaderToSPV(EShaderStage InShaderStage, const std::string& InShaderString, const std::string& InEntry, ShaderCompileResult& InOutResult);
		bool CompileShaderToSPV(VulkanShader* InShader, ShaderCompileResult& InOutResult);
		void AddWaitCompile(VulkanShader* InShader);
	private:
		std::set<VulkanShader*> mWaitCompileShaders; //todo muilt thread
	};
}