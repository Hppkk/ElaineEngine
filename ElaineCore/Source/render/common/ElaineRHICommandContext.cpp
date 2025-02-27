#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHICommandList.h"

namespace Elaine
{
	RHICommandContext::RHICommandContext()
	{
		mRHICmdListMgr = new RHICommandListManager(this);
	}

	RHICommandContext::~RHICommandContext()
	{
		SAFE_DELETE(mRHICmdListMgr);
	}
}