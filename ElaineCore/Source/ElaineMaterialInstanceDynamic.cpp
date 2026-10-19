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
		mOverridedInts.clear();
		mOverridedBools.clear();
		++mRevision;
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
		++mRevision;
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
		mOverridedScalars[InParamName.GetHashValue()] = InValue;
		++mRevision;
	}

	float MaterialInstanceDynamic::GetScalar(const Name& InParamName, float InDefault) const
	{
		auto it = mOverridedScalars.find(InParamName.GetHashValue());
		if (it != mOverridedScalars.end()) return it->second;
		if (!mSource.isNull() && mSource->HasScalar(InParamName.GetHashValue())) return mSource->GetScalar(InParamName.GetHashValue());
		return InDefault;
	}

	bool MaterialInstanceDynamic::HasScalarOverride(const Name& InParamName) const
	{
		return mOverridedScalars.find(InParamName.GetHashValue()) != mOverridedScalars.end();
	}

	void MaterialInstanceDynamic::SetVector(const Name& InParamName, const Vector4& InValue)
	{
		mOverridedVectors[InParamName.GetHashValue()] = InValue;
		++mRevision;
	}

	Vector4 MaterialInstanceDynamic::GetVector(const Name& InParamName, const Vector4& InDefault) const
	{
		auto it = mOverridedVectors.find(InParamName.GetHashValue());
		if (it != mOverridedVectors.end()) return it->second;
		if (!mSource.isNull() && mSource->HasVector(InParamName.GetHashValue())) return mSource->GetVector(InParamName.GetHashValue());
		return InDefault;
	}

	bool MaterialInstanceDynamic::HasVectorOverride(const Name& InParamName) const
	{
		return mOverridedVectors.find(InParamName.GetHashValue()) != mOverridedVectors.end();
	}

	void MaterialInstanceDynamic::SetInt(const Name& InParamName, int32_t InValue) { mOverridedInts[InParamName.GetHashValue()] = InValue; ++mRevision; }
	int32_t MaterialInstanceDynamic::GetInt(const Name& InParamName, int32_t InDefault) const { auto It = mOverridedInts.find(InParamName.GetHashValue()); return It != mOverridedInts.end() ? It->second : (!mSource.isNull() ? mSource->GetInt(InParamName.GetHashValue(), InDefault) : InDefault); }
	bool MaterialInstanceDynamic::HasIntOverride(const Name& InParamName) const { return mOverridedInts.find(InParamName.GetHashValue()) != mOverridedInts.end(); }
	void MaterialInstanceDynamic::SetBool(const Name& InParamName, bool InValue) { mOverridedBools[InParamName.GetHashValue()] = InValue; ++mRevision; }
	bool MaterialInstanceDynamic::GetBool(const Name& InParamName, bool InDefault) const { auto It = mOverridedBools.find(InParamName.GetHashValue()); return It != mOverridedBools.end() ? It->second : (!mSource.isNull() ? mSource->GetBool(InParamName.GetHashValue(), InDefault) : InDefault); }
	bool MaterialInstanceDynamic::HasBoolOverride(const Name& InParamName) const { return mOverridedBools.find(InParamName.GetHashValue()) != mOverridedBools.end(); }

	MaterialParamSnapshot MaterialInstanceDynamic::CreateSnapshot() const
	{
		MaterialParamSnapshot Snapshot;
		Snapshot.Source = mSource;
		Snapshot.OverridedTextures = mOverridedTextures;
		Snapshot.OverridedScalars = mOverridedScalars;
		Snapshot.OverridedVectors = mOverridedVectors;
		Snapshot.OverridedInts = mOverridedInts;
		Snapshot.OverridedBools = mOverridedBools;
		const uint64_t LayoutRevision = mSource.isNull() ? 0 : mSource->GetLayoutRevision();
		Snapshot.Revision = (mRevision * 0x9E3779B185EBCA87ull) ^ LayoutRevision;
		return Snapshot;
	}
}
