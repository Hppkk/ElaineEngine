#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMaterialInterface.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineResourceBase.h"

namespace Elaine
{
	enum VertexRate
	{
		PerVertex,
		PerInstance
	};

	struct VertexAttributeDesc
	{
		VertexSemantic Semantic;          // POSITION / NORMAL / TEXCOORD / COLOR / CUSTOM
		uint32 Index;           // TEXCOORD0 / TEXCOORD1
		VertexElementType Format;       // Float3 / Float2 / UByte4N ...
		uint32 Offset;          // 在一个 vertex 里的偏移
	};

	struct VertexBufferDesc
	{
		uint32 Stride;
		VertexRate Rate;
		std::vector<VertexAttributeDesc> Attributes;
	};

	struct VertexLayoutDesc
	{
		std::vector<VertexBufferDesc> Buffers;
	};

	struct MaterialSlot
	{
		uint64 SlotHash;
		std::string SlotName;
	};

	class ElaineCoreExport SubMesh
	{
	public:
		SubMesh() = default;
		~SubMesh() = default;

		/// 索引起始位置（Mesh Index Buffer 中）
		uint32 StartIndex = 0;

		/// 索引数量
		uint32 IndexCount = 0;

		/// 顶点起始（非 0 时用于 BaseVertex）
		int32 BaseVertex = 0;

		uint32 MaterialSlot;
		MaterialInterface* MaterialInstance = nullptr;

		Name DebugName;

	public:

		inline bool IsValid() const
		{
			return IndexCount > 0 && MaterialInstance != nullptr;
		}

	};

	class ElaineCoreExport Mesh : public ResourceBase
	{
	public:
		Mesh() = default;
		Mesh(ResourceManager* pManager, const std::string& res_name);
		virtual ~Mesh() override;
		uint32 GetSubMeshCount() const;
	protected:
		virtual bool LoadImpl() override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;
		virtual void ResourceArrivedImpl() override;
	private:
		VertexLayoutDesc mVertexLayout;
		std::vector<SubMesh*> mSubMeshes;
		std::vector<std::vector<uint8_t>> mCPUVertexBuffers;
		std::vector<uint32_t> mCPUIndexBuffer;
		std::vector<RHIBuffer*> mGPUVertexBuffers;
		std::vector<MaterialSlot> mMaterialSlots;
		RHIBuffer* mIndexBuffer = nullptr;
#ifdef _HAS_EDITOR_
		friend class MeshConverter;
#endif
	};

	using MeshPtr = ResourcePtr<Mesh>;
#ifdef _HAS_EDITOR_
	void ElaineCoreExport ImportMeshFromFile(const std::string& srcPath, const std::string& destRelPath);
#endif
}