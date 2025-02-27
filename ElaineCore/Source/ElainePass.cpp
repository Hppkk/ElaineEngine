#include "ElainePrecompiledHeader.h"
#include "ElainePass.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	Pass::Pass(PassType InType)
		: mPassType(InType)
	{

	}
	void Pass::AppendVsMacros(const std::string& InMacrosString)
	{
		if (InMacrosString.empty())
			return;

		mVsMacros.append(InMacrosString);
	}

	void Pass::AppendPsMacros(const std::string& InMacrosString)
	{
		if (InMacrosString.empty())
			return;

		mPsMacros.append(InMacrosString);
	}

	void Pass::CompilePipeline()
	{
		mPipeline = GetDynamicRHI()->GetDefaultCommandContext()->RHICreateGfxPipeline(mRHIDesc);
	}
}