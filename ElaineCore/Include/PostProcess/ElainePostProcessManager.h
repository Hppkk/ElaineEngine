#pragma once
#include "ElaineCorePrerequirements.h"
#include "PostProcess/ElainePostProcessUtils.h"

namespace Elaine
{
	class PostProcessBase;

	class ElaineCoreExport PostProcessChain
	{
	public:
		PostProcessChain();
		~PostProcessChain();
		void Initialize(const std::string& InPath);
	private:
		PostProcessBase* mPostList[PP_Count] = { };
	};
}