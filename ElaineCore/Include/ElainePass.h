#pragma once

namespace Elaine
{

	enum PassType :unsigned int
	{
		CustomPass0,	//�Զ���pass
		CustomPass1,	//�Զ���pass
		CustomPass2,	//�Զ���pass
		CustomPass3,	//�Զ���pass
		CustomPass4,	//�Զ���pass
		NormalPass,
		ShadowPass,
		TransparentNormalPass,
		TransparentDepthPass,	//͸����һ��pass
		TransparentRenderPass,	//͸���ڶ���pass

		TransparentFrontPass, //͸��ֻ��Ⱦ����pass
		TransparentBackPass, //͸��ֻ��Ⱦ����pass
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