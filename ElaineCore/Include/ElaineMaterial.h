#pragma once
#include "ElaineResourceBase.h"
#include "ElainePass.h"
#include "render/common/ElaineRHIProtocol.h"

namespace Elaine
{

	class ElaineCoreExport Material : public ResourceBase
	{
	public:
		Material();
		virtual ~Material();
		virtual void loadImpl() override;
		virtual	void unloadImpl() override;

		void BeginPass(PassType InPassType);
		void EndPass(PassType InPassType);
		Pass* GetPass(PassType InPassType);
		bool IsRegisterPass(PassType InPassType);
	private:
		Pass* mPassMap[PassCount];
		std::string mVsShaderPath;
		std::string mPsShaderPath;
		std::string mVsShaderCode;
		std::string mPsShaderCode;
	public:
		
	};
}
