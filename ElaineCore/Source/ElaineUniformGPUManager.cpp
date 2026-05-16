#include "ElainePrecompiledHeader.h"
#include "ElaineUniformGPUManager.h"

namespace Elaine
{
	void SemanticsRegister::Initialize()
	{
		RegisterTextureSemantics();
		RegisterVertexSemantics();
	}

	void SemanticsRegister::RegisterTextureSemantics()
	{
#define REG_TEX_SEM(NAME) \
	mTextureSemanticsStr.emplace(NAME, Name(STR_NAME(NAME))); \
	mTextureSemantics.emplace(Name(STR_NAME(NAME)), NAME);
		REG_TEX_SEM(BaseColor);
		REG_TEX_SEM(Normal);
		REG_TEX_SEM(Specular);
		REG_TEX_SEM(Ambient);
		REG_TEX_SEM(Metallic);
		REG_TEX_SEM(Roughness);

		// Virtual Texture semantics
		REG_TEX_SEM(VT_Indirection);
		REG_TEX_SEM(VT_PhysicalAtlas_BaseColor);
		REG_TEX_SEM(VT_PhysicalAtlas_Normal);
		REG_TEX_SEM(VT_PhysicalAtlas_RMA);
		REG_TEX_SEM(VT_PhysicalAtlas_Emissive);
	}

	void SemanticsRegister::RegisterVertexSemantics()
	{
#define REG_VERTEX_SEM(NAME) \
	mVertexSemanticStr.emplace(NAME, Name(STR_NAME(NAME))); \
	mVertexSemantics.emplace(Name(STR_NAME(NAME)), NAME);

		REG_VERTEX_SEM(POSITION);
		REG_VERTEX_SEM(NORMAL);
		REG_VERTEX_SEM(TANGENT);
		REG_VERTEX_SEM(BITANGENT);
		REG_VERTEX_SEM(TEXCOORD);
		REG_VERTEX_SEM(COLOR);
		REG_VERTEX_SEM(BLEND_WEIGHTS);
		REG_VERTEX_SEM(INSTANCE_MATRIX);
		REG_VERTEX_SEM(INSTANCE_COLOR);
		REG_VERTEX_SEM(CUSTOM);
	}

	const Name& SemanticsRegister::GetName(TextureSemantics InTextureSemantice)
	{
		return mTextureSemanticsStr[InTextureSemantice];
	}

	TextureSemantics SemanticsRegister::GetSemantics(const Name& InTextureSemantice)
	{
		return mTextureSemantics[InTextureSemantice];
	}

	const Name& SemanticsRegister::GetName(VertexSemantic InTextureSemantice)
	{
		return mVertexSemanticStr[InTextureSemantice];
	}

	VertexSemantic SemanticsRegister::GetVertexSemantic(const Name& InSemantice)
	{
		return mVertexSemantics[InSemantice];
	}
}