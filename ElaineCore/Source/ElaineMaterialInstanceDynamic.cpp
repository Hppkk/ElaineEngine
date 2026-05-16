#include "ElainePrecompiledHeader.h"
#include "ElaineMaterialInstanceDynamic.h"
#include "ElaineMaterialParamSnapshot.h"
#include "ElaineMaterialManager.h"

namespace Elaine
{
	MaterialInstanceDynamic::MaterialInstanceDynamic()
	{
		mMaterialType = MaterialDynamic;
	}

	MaterialInstanceDynamic::~MaterialInstanceDynamic()
	{
	}

	void MaterialInstanceDynamic::ChangeMaterial(const std::string& InPath)
	{
		if (InPath == mPath)
			return;

		mPath = InPath;
		mSource = MaterialInstanceStaticManager::instance()->GetResource(InPath);
		
		// 清空覆盖的参数，因为切换了材质
		mOverridedTextures.clear();
		mOverridedScalars.clear();
		mOverridedVectors.clear();
	}

	bool MaterialInstanceDynamic::IsReady() const
	{
		if (mSource.isNull())
			return false;
		
		// 检查MaterialInstanceStatic是否加载完成
		if (!mSource->IsLoaded())
			return false;
		
		// 检查底层Material是否加载完成
		const MaterialPtr& originMat = mSource->GetOriginMaterial();
		if (originMat.isNull())
			return false;
			
		return originMat->IsLoaded();
	}

	void MaterialInstanceDynamic::SetTexture(TextureSemantics InSemantics, const TexturePtr& InTexture)
	{
		mOverridedTextures[InSemantics] = InTexture;
	}

	TexturePtr MaterialInstanceDynamic::GetTexture(TextureSemantics InSemantics) const
	{
		auto it = mOverridedTextures.find(InSemantics);
		if (it != mOverridedTextures.end())
			return it->second;
		
		if (!mSource.isNull())
			return mSource->GetTexture(InSemantics);

		return TexturePtr();
	}

	bool MaterialInstanceDynamic::HasTextureOverride(TextureSemantics InSemantics) const
	{
		return mOverridedTextures.find(InSemantics) != mOverridedTextures.end();
	}

	void MaterialInstanceDynamic::SetScalar(const Name& InParamName, float InValue)
	{
		//mOverridedScalars[InParamName.GetHash()] = InValue;
	}

	float MaterialInstanceDynamic::GetScalar(const Name& InParamName, float InDefault) const
	{
		//auto it = mOverridedScalars.find(InParamName.GetHash());
		//if (it != mOverridedScalars.end())
		//	return it->second;
		return InDefault;
	}

	bool MaterialInstanceDynamic::HasScalarOverride(const Name& InParamName) const
	{
		//return mOverridedScalars.find(InParamName.GetHash()) != mOverridedScalars.end();
		return true;
	}

	void MaterialInstanceDynamic::SetVector(const Name& InParamName, const Vector4& InValue)
	{
		//mOverridedVectors[InParamName.GetHash()] = InValue;
	}

	Vector4 MaterialInstanceDynamic::GetVector(const Name& InParamName, const Vector4& InDefault) const
	{
		//auto it = mOverridedVectors.find(InParamName.GetHash());
		//if (it != mOverridedVectors.end())
		//	return it->second;
		return InDefault;
	}

	bool MaterialInstanceDynamic::HasVectorOverride(const Name& InParamName) const
	{
		//return mOverridedVectors.find(InParamName.GetHash()) != mOverridedVectors.end();
		return true;
	}

	MaterialParamSnapshot MaterialInstanceDynamic::CreateSnapshot() const
	{
		MaterialParamSnapshot Snapshot;
		Snapshot.Source = mSource;
		Snapshot.OverridedTextures = mOverridedTextures;
		Snapshot.OverridedScalars = mOverridedScalars;
		Snapshot.OverridedVectors = mOverridedVectors;
		return Snapshot;
	}
}