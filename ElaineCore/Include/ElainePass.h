#pragma once

namespace Elaine
{

	enum PassType :unsigned int
	{
		CustomPass0,	//自定义pass
		CustomPass1,	//自定义pass
		CustomPass2,	//自定义pass
		CustomPass3,	//自定义pass
		CustomPass4,	//自定义pass
		NormalPass,
		ShadowPass,
		TransparentNormalPass,
		TransparentDepthPass,	//透明第一次pass
		TransparentRenderPass,	//透明第二次pass

		TransparentFrontPass, //透明只渲染正面pass
		TransparentBackPass, //透明只渲染背面pass
		PassCount,
	};



	class ElaineCoreExport Pass
	{
	public:
		Pass(PassType InType);
		const std::string& GetVsMacros() const { return mVsMacros; }
		const std::string& GetPsMacros() const { return mPsMacros; }
		void AppendVsMacros(const std::string& InMacrosString);
		void AppendPsMacros(const std::string& InMacrosString);
		void CompilePipeline();
		GRAPHICS_PIPELINE_STATE_DESC& GetGfxState() { return mRHIDesc; }
		RHIPipeline* GetRHIPipeline()const { return mPipeline; }
	private:
		std::string		mVsMacros;
		std::string		mPsMacros;
		PassType		mPassType = PassType::NormalPass;
		RHIPipeline*	mPipeline = nullptr;
	public:
		GRAPHICS_PIPELINE_STATE_DESC mRHIDesc;
	};
}