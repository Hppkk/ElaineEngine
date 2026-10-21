#pragma once
#include "render/vulkan/ElaineVulkanTypes.h"

namespace VulkanRHI
{
	//enum class MaterialParameterType : uint8_t
	//{
	//	Unknown,
	//	Scalar,
	//	Vector,
	//	Integer,
	//	Boolean,
	//	Matrix,
	//	Array
	//};

	//struct MaterialParameterInfo
	//{
	//	std::string name;
	//	uint64_t nameHash = 0;
	//	MaterialParameterType parameterType = MaterialParameterType::Unknown;
	//	uint32_t offset = 0;
	//	uint32_t size = 0;
	//	uint32_t arrayCount = 1;
	//	uint32_t set = 0;
	//	uint32_t binding = 0;
	//};
	class VulkanDevice;
	class VulkanPipeline;
	class VulkanShader;

	struct DescriptorBindingInfo
	{
		uint32_t set;
		uint32_t binding;
		VkDescriptorType type;
		uint32_t count;
		VkShaderStageFlags stageFlags;
		size_t blockSize;
		bool isArray;
        bool isColor2DSampler = false;
		std::string name;
		RHIResourceVisibility mVisibility;
	};

	struct PushConstantRange
	{
		uint32_t offset;
		uint32_t size;
		VkShaderStageFlags stageFlags;
	};

	struct ShaderReflectionData
	{
		std::unordered_map<uint32_t, std::vector<DescriptorBindingInfo>> descriptorSets;
		std::vector<PushConstantRange> pushConstants;
		std::vector<VkVertexInputAttributeDescription> inputAttributes;
	};

	struct PipelineReflectionData
	{
		std::unordered_map<uint32_t, std::vector<DescriptorBindingInfo>> descriptorSets;
		std::vector<PushConstantRange> pushConstants;
		std::vector<VkVertexInputAttributeDescription> inputAttributes;
		std::vector<RHIPipeline::MaterialParameterInfo> materialParameters;
		VulkanPipeline* mPipeline = nullptr;
	};

    struct PreparedShader final : Elaine::RHIShaderCompileResult
    {
        std::vector<uint32_t> Spirv;
        PipelineReflectionData Reflection;
        std::vector<VkVertexInputBindingDescription> InputBindings;
    };
	//struct ElaineCoreExport ShaderCompileResult
	//{
	//	ShaderCompileResult() = default;
	//	~ShaderCompileResult();

	//	void SetCode(const uint32_t* InCode, size_t InSize);

	//	std::vector<uint32_t> mSpirvCode;
	//	ShaderReflectionData mReflectionData;
	//};

	struct SPVCodeDesc
	{
		SPVCodeDesc() = default;
		~SPVCodeDesc();
		void SetCode(const uint32_t* InCode, size_t InSize);

		uint32_t* mSPVCodeContent = nullptr;
		size_t mContentSize = 0u;
	};

	class ShaderCompileFinishListener
	{
	public:
		//virtual void OnShaderCompileFinish(const ShaderCompileResult& InResult) = 0;
	};

	std::string GenerateShaderHashKey(const std::string& InShaderCode);

	class ElaineCoreExport VulkanShader :public Elaine::RHIShader
	{
	public:
		VulkanShader(VulkanDevice* InDevice, const std::string& InShaderCode, EShaderStage InStage, const std::string& InHashKey = "");
		~VulkanShader();
		std::string GetHash() const { return mHashKey; }
		const VkPipelineShaderStageCreateInfo& GetShaderStageCreateInfo() const { return mShaderStageInfo; }
		void CopyFrom(const VulkanShader& InShader);
		//virtual void OnShaderCompileFinish(const ShaderCompileResult& InResult) override;
		bool IsShaderCompiled() const { return mbCompiled; }
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return mDescriptorSetLayouts; }
		const std::vector<VkDescriptorSet>& GetDescriptorSet() const { return mDescriptorSets; }
		const std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescriptions()const { return mInputAttributeDescriptions; }
		const std::vector<VkVertexInputBindingDescription>& GetVertexInputBindingDescription() const { return mVertexInputBindingDescs; }
		void GetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& InOutDescriptorSetLayouts);
		const SPVCodeDesc& GetSPVCodeDesc() const { return mSPVCodeDesc; }
		VkShaderStageFlagBits GetVkShaderStageFlag() { return mShaderStageFlag; }
	private:
		void CreateShaderModule();
		//void GenerateDescriptorSetLayout(const ShaderCompileResult& InResult);
	protected:
		std::string mHashKey;
		VkShaderModule mShaderModule = nullptr;
		VkShaderStageFlagBits mShaderStageFlag = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		VkPipelineShaderStageCreateInfo mShaderStageInfo;
		VulkanDevice* mVkDevice = nullptr;
		EShaderStage mRHIShaderStage;
		//uint32_t* mVkSPVCode = nullptr;
		SPVCodeDesc mSPVCodeDesc;
		std::vector<VkDescriptorSet> mDescriptorSets;
		std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
		std::vector<VkVertexInputAttributeDescription> mInputAttributeDescriptions;
		std::vector<VkVertexInputBindingDescription> mVertexInputBindingDescs;
		VulkanPipeline* mPipeline = nullptr;
        std::shared_ptr<const PreparedShader> mPrepared;
		bool mbCompiled = false;
		friend class VulkanShaderCompileManager;
		friend class VulkanShaderManager;
	};


	class ElaineCoreExport VulkanShaderManager
	{
	public:
		VulkanShaderManager(VulkanDevice* InDevice);
		~VulkanShaderManager();
		VulkanShader* GetOrCreateShader(const std::string& InShaderCode, EShaderStage InStage);
		VulkanShader* CreateShader(const std::string& InShaderCode, EShaderStage InStage, VulkanPipeline* InPipeline, const std::string& InShaderPath, std::shared_ptr<const PreparedShader> Prepared = {});
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
        static std::shared_ptr<const PreparedShader> PrepareCPU(const std::string& Code, const std::string& Path, EShaderStage Stage);
        static void ReflectCPU(VulkanShader* Shader, PipelineReflectionData& Reflection);
		class Private;
	public:
		VulkanShaderCompileManager(VulkanDevice* InDevice);
		~VulkanShaderCompileManager();
		void PreprocessShader(VulkanShader* InShader);
		//bool CompileShaderToSPV(EShaderStage InShaderStage, const std::string& InShaderString, const std::string& InEntry, ShaderCompileResult& InOutResult);
		bool CompileShaderToSPV(VulkanShader* InShader);
		bool PostCompileShader(VulkanShader* InVertexShader, VulkanShader* InFragmentShader);
		bool GeneratePipelineDescriptors(PipelineReflectionData& InPipelineRefData);
		void AddWaitCompile(VulkanShader* InShader);
		bool CompilePipeline(VulkanPipeline* InPipeline);
	private:
		//void ReflectShaderModule(ShaderCompileResult& InOutResult, EShaderStage InShaderStage);
	private:
		std::set<VulkanShader*> mWaitCompileShaders; //todo muilt thread
		VulkanDevice* mDevice = nullptr;
	};
}
