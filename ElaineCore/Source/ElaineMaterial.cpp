#include "ElainePrecompiledHeader.h"
#include "ElaineMaterial.h"

namespace Elaine
{
	Material::Material()
	{
		Memory::MemoryZero(mPassMap,sizeof(size_t) * (unsigned int)PassType::PassCount);
	}

	Material::~Material()
	{
		for (size_t Index = 0; Index < PassType::PassCount; ++Index)
		{
			if (mPassMap[Index])
			{
				SAFE_DELETE(mPassMap[Index]);
			}
		}
	}

	void Material::loadImpl()
	{

	}

	void Material::unloadImpl()
	{

	}

	void Material::BeginPass(PassType InPassType)
	{
		if (IsRegisterPass(InPassType))
			return;
		mPassMap[InPassType] = new Pass(InPassType);
		mPassMap[InPassType]->mRHIDesc.mVSShaderCode = mVsShaderCode;
		mPassMap[InPassType]->mRHIDesc.mPSShaderCode = mPsShaderCode;
	}

	void Material::EndPass(PassType InPassType)
	{
		mPassMap[InPassType]->CompilePipeline();
	}

	Pass* Material::GetPass(PassType InPassType)
	{
		return mPassMap[InPassType];
	}

	bool Material::IsRegisterPass(PassType InPassType)
	{
		return mPassMap[InPassType] != nullptr;
	}
}